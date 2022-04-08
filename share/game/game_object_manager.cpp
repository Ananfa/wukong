/*
 * Created by Xianke Liu on 2021/1/15.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "corpc_routine_env.h"
#include "corpc_pubsub.h"
#include "game_object_manager.h"
#include "redis_pool.h"
#include "redis_utils.h"
#include "game_delegate.h"
#include "game_center.h"
#include "client_center.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

void GameObjectManager::init() {
    // 启动全服事件处理协程
    RoutineEnvironment::startCoroutine(globalEventHandleRoutine, this);

    g_GameCenter.registerEventQueue(_eventQueue);
}

void *GameObjectManager::globalEventHandleRoutine(void * arg) {
    // 注意：GameObjectManager对象是在服务器启动时一遍启动，并随着服务器进程关闭时销毁，因此不考虑GameObjectManager本身的delete处理
    GameObjectManager *self = (GameObjectManager *)arg;

    // 初始化pipe readfd
    int readFd = self->_eventQueue->getReadFd();
    co_register_fd(readFd);
    co_set_timeout(readFd, -1, 1000);
    
    int ret;
    std::vector<char> buf(1024);
    while (true) {
        // 等待处理信号
        ret = (int)read(readFd, &buf[0], 1024);
        assert(ret != 0);
        if (ret < 0) {
            if (errno == EAGAIN) {
                continue;
            } else {
                // 管道出错
                ERROR_LOG("GameObjectManager::globalEventHandleRoutine -- read from pipe fd %d ret %d errno %d (%s)\n",
                       readFd, ret, errno, strerror(errno));
                
                // TODO: 如何处理？退出协程？
                // sleep 10 milisecond
                msleep(10);
            }
        }
        
        // 分派事件处理
        std::shared_ptr<wukong::pb::GlobalEventMessage> message = self->_eventQueue->pop();
        while (message) {
            Event e(message->topic().c_str());
            e.setParam("data", message->data());
            self->_emiter.fireEvent(e);
            
            message = self->_eventQueue->pop();
        }
    }
    
    return NULL;
}

void GameObjectManager::shutdown() {
    if (_shutdown) {
        return;
    }

    _shutdown = true;

    for (auto &gameObj : _roleId2GameObjectMap) {
        gameObj.second->stop();
    }

    _roleId2GameObjectMap.clear();
}

size_t GameObjectManager::roleCount() {
    return _roleId2GameObjectMap.size();
}

bool GameObjectManager::existRole(RoleId roleId) {
    return _roleId2GameObjectMap.find(roleId) != _roleId2GameObjectMap.end();
}

std::shared_ptr<GameObject> GameObjectManager::getGameObject(RoleId roleId) {
    auto it = _roleId2GameObjectMap.find(roleId);
    if (it == _roleId2GameObjectMap.end()) {
        return nullptr;
    }

    return it->second;
}

bool GameObjectManager::loadRole(RoleId roleId, ServerId gatewayId) {
    // 判断GameObject是否已经存在
    if (existRole(roleId)) {
        ERROR_LOG("GameObjectManager::loadRole -- role[%llu] game object already exist\n", roleId);
        return false;
    }

    // 查询记录对象位置
    //   若记录对象不存在，分配（负载均衡）record服
    // 尝试设置location
    // 设置成功时加载玩家数据（附带创建记录对象）
    // 重新设置location过期（若这里不设置，后面创建的GameObject会等到第一次心跳时才销毁）
    // 创建GameObject
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("GameObjectManager::loadRole -- role[%llu] connect to cache failed\n", roleId);
        return false;
    }

    ServerId recordId;
    RedisAccessResult res = RedisUtils::GetRecordAddress(cache, roleId, recordId);
    switch (res) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GameObjectManager::loadRole -- role[%llu] get record failed for db error\n", roleId);
            return false;
        }
        case REDIS_FAIL: {
            if (!g_ClientCenter.randomRecordServer(recordId)) {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                ERROR_LOG("GameObjectManager::loadRole -- role[%llu] random record server failed\n", roleId);
                return false;
            }
        }
    }

    // 生成lToken（直接用当前时间来生成）
    struct timeval t;
    gettimeofday(&t, NULL);
    std::string lToken = std::to_string((t.tv_sec % 1000) * 1000000 + t.tv_usec);

    res = RedisUtils::SetGameObjectAddress(cache, roleId, _type, _id, lToken);
    switch (res) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GameObjectManager::loadRole -- role[%llu] set location failed\n", roleId);
            return false;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("GameObjectManager::loadRole -- role[%llu] set location failed for already set\n", roleId);
            return false;
        }
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 向Record服发加载数据RPC
    std::string roleData;
    ServerId serverId; // 角色所属区服号
    UserId userId;
    if (!g_RecordClient.loadRoleData(recordId, roleId, userId, lToken, serverId, roleData)) {
        ERROR_LOG("GameObjectManager::loadRole -- role[%llu] load role data failed\n", roleId);
        return false;
    }

    // 这里是否需要加一次对location超时设置，避免加载数据时location过期？
    // 如果这里不加检测，创建的GameObject会等到第一次心跳设置location时才销毁
    // 如果加检测会让登录流程延长
    // cache = g_MysqlPoolManager.getCoreCache()->take();
    // reply = (redisReply *)redisCommand(cache, "EXPIRE Location:%d %d", roleId, 60);
    // if (reply->integer != 1) {
    //     // 设置超时失败，可能是key已经过期
    //     freeReplyObject(reply);
    //     g_MysqlPoolManager.getCoreCache()->put(cache, false);
    //     ERROR_LOG("InnerLobbyServiceImpl::initRole -- user %d role %d load role data failed for cant set location expire\n", userId, roleId);
    //     return;
    // }
    // freeReplyObject(reply);
    // g_MysqlPoolManager.getCoreCache()->put(cache, false);

    // 创建GameObject
    if (_shutdown) {
        WARN_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] already shutdown\n", userId, roleId);
        return false;
    }

    if (_roleId2GameObjectMap.find(roleId) != _roleId2GameObjectMap.end()) {
        ERROR_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] game object already exist\n", userId, roleId);
        return false;
    }

    if (!g_GameDelegate.getCreateGameObjectHandle()) {
        ERROR_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] not set CreateGameObjectHandle\n", userId, roleId);
        return false;
    }

    // 创建GameObject
    auto obj = g_GameDelegate.getCreateGameObjectHandle()(userId, roleId, serverId, lToken, this, roleData);

    if (!obj) {
        ERROR_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] create game object failed\n", userId, roleId);
        return false;
    }

    // 设置gateway和record stub
    if (gatewayId != 0) {
        // 这里可以不用设置gateway stub
        if (!obj->setGatewayServerStub(gatewayId)) {
            ERROR_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] can't set gateway stub\n", userId, roleId);
            return false;
        }
    }

    if (!obj->setRecordServerStub(recordId)) {
        ERROR_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] can't set record stub\n", userId, roleId);
        return false;
    }

    _roleId2GameObjectMap.insert(std::make_pair(roleId, obj));
    obj->start();

    return true;
}

void GameObjectManager::leaveGame(RoleId roleId) {
    auto it = _roleId2GameObjectMap.find(roleId);
    assert(it != _roleId2GameObjectMap.end());

    it->second->stop();
    _roleId2GameObjectMap.erase(it);
}

uint32_t GameObjectManager::regGlobalEventHandle(const std::string &name, EventHandle handle) {
    return _emiter.addEventHandle(name, handle);
}

void GameObjectManager::unregGlobalEventHandle(uint32_t refId) {
    _emiter.removeEventHandle(refId);
}

void GameObjectManager::fireGlobalEvent(const Event &event) {
    std::string data;
    if (!event.getParam("data", data)) {
        ERROR_LOG("GameObjectManager::fireGlobalEvent -- event hasn't data param\n");
    }

    fireGlobalEvent(event.getName(), data);
}

void GameObjectManager::fireGlobalEvent(const std::string &topic, const std::string &data) {
    wukong::pb::GlobalEventMessage message;
    message.set_topic(topic);
    message.set_data(data);

    std::string pData;
    message.SerializeToString(&pData);

    PubsubService::Publish("GEvent", pData);
}
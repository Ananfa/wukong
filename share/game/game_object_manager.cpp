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
#if 0
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
    geventDispatcher_.init(g_GameCenter.getGlobalEventListener());
}

void GameObjectManager::shutdown() {
    if (shutdown_) {
        return;
    }

    shutdown_ = true;
    
    // 若不清dispatcher，会导致shared_ptr循环引用问题
    geventDispatcher_.clear();

    for (auto &gameObj : roleId2GameObjectMap_) {
        gameObj.second->stop();
    }

    roleId2GameObjectMap_.clear();
}

size_t GameObjectManager::roleCount() {
    return roleId2GameObjectMap_.size();
}

bool GameObjectManager::existRole(RoleId roleId) {
    return roleId2GameObjectMap_.find(roleId) != roleId2GameObjectMap_.end();
}

std::shared_ptr<GameObject> GameObjectManager::getGameObject(RoleId roleId) {
    auto it = roleId2GameObjectMap_.find(roleId);
    if (it == roleId2GameObjectMap_.end()) {
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

    res = RedisUtils::SetGameObjectAddress(cache, roleId, type_, id_, lToken);
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
    DEBUG_LOG("GameObjectManager::loadRole -- load from record, userId:%d\n", userId);

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
    if (shutdown_) {
        WARN_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] already shutdown\n", userId, roleId);
        return false;
    }

    if (roleId2GameObjectMap_.find(roleId) != roleId2GameObjectMap_.end()) {
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
    if (gatewayId == 0) {
        // 先尝试查找玩家的gatewayId
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("GameObjectManager::loadRole -- role[%llu] connect to cache failed when get session\n", roleId);
            return false;
        }

        ServerId gwid = 0;
        std::string gToken;
        RoleId rid = 0;
        switch (RedisUtils::GetSession(cache, userId, gwid, gToken, rid)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("GameObjectManager::loadRole -- role[%llu] get session failed\n", roleId);
                return false;
            }
            case REDIS_FAIL: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("GameObjectManager::loadRole -- role[%llu] get session failed for return type invalid\n", roleId);
                return false;
            }
            default: { // REDIS_SUCCESS
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                if (rid == roleId) {
                    gatewayId = gwid;
                }
                break;
            }
        }
    }

    // 当gatewayId为0时可以不用设置gateway stub（允许离线加载角色）
    if (gatewayId != 0) {
        if (!obj->setGatewayServerStub(gatewayId)) {
            ERROR_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] can't set gateway stub\n", userId, roleId);
            return false;
        }

        // 通知gateway更新gameobj地址
        //obj->reportGameObjectPos();
    }

    if (!obj->setRecordServerStub(recordId)) {
        ERROR_LOG("GameObjectManager::loadRole -- user[%llu] role[%llu] can't set record stub\n", userId, roleId);
        return false;
    }

    roleId2GameObjectMap_.insert(std::make_pair(roleId, obj));
    obj->start();

    return true;
}

void GameObjectManager::leaveGame(RoleId roleId) {
    auto it = roleId2GameObjectMap_.find(roleId);
    assert(it != roleId2GameObjectMap_.end());

    it->second->stop();
    roleId2GameObjectMap_.erase(it);
}

//void GameObjectManager::offline(RoleId roleId) {
//    auto it = roleId2GameObjectMap_.find(roleId);
//    assert(it != roleId2GameObjectMap_.end());
//
//    it->second->onOffline();
//}

uint32_t GameObjectManager::regGlobalEventHandle(const std::string &name, EventHandle handle) {
    return geventDispatcher_.regGlobalEventHandle(name, handle);
}

void GameObjectManager::unregGlobalEventHandle(uint32_t refId) {
    geventDispatcher_.unregGlobalEventHandle(refId);
}

void GameObjectManager::fireGlobalEvent(const Event &event) {
    geventDispatcher_.fireGlobalEvent(event);
}
#endif
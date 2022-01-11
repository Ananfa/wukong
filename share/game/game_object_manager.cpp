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
#include "game_object_manager.h"
#include "game_center.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

void GameObjectManager::init() {
    
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

size_t GameObjectManager::size() {
    return _roleId2GameObjectMap.size();
}

bool GameObjectManager::exist(RoleId roleId) {
    return _roleId2GameObjectMap.find(roleId) != _roleId2GameObjectMap.end();
}

std::shared_ptr<GameObject> GameObjectManager::getGameObject(RoleId roleId) {
    auto it = _roleId2GameObjectMap.find(roleId);
    if (it == _roleId2GameObjectMap.end()) {
        return nullptr;
    }

    return it->second;
}

bool GameObjectManager::loadRole(UserId userId, RoleId roleId, ServerId gatewayId) {
    // 判断GameObject是否已经存在
    if (exist(roleId)) {
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d game object already exist\n", userId, roleId);
        return false;
    }

    // 查询记录对象位置
    //   若记录对象不存在，分配（负载均衡）record服
    // 尝试设置location
    // 设置成功时加载玩家数据（附带创建记录对象）
    // 重新设置location过期（若这里不设置，后面创建的GameObject会等到第一次心跳时才销毁）
    // 创建GameObject
    redisContext *cache = g_GameCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d connect to cache failed\n", userId, roleId);
        return false;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "HGET Record:%d loc", roleId);
    if (!reply) {
        g_GameCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d get record failed for db error\n", userId, roleId);
        return false;
    }

    ServerId recordId;
    if (reply->type == REDIS_REPLY_STRING) {
        recordId = std::stoi(reply->str);
    } else if (reply->type == REDIS_REPLY_NIL) {
        if (!g_GameCenter.randomRecordServer(recordId)) {
            freeReplyObject(reply);
            g_GameCenter.getCachePool()->proxy.put(cache, false);
            ERROR_LOG("GameObjectManager::loadRole -- user %d role %d random record server failed\n", userId, roleId);
            return false;
        }
    } else {
        freeReplyObject(reply);
        g_GameCenter.getCachePool()->proxy.put(cache, false);
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d get record failed for invalid data type\n", userId, roleId);
        return false;
    }

    freeReplyObject(reply);

    // 生成lToken（直接用当前时间来生成）
    struct timeval t;
    gettimeofday(&t, NULL);
    uint32_t lToken = (t.tv_sec % 1000) * 1000000 + t.tv_usec;

    // 尝试设置location
    // TODO: loc的格式应该包含游戏服类型
    if (g_GameCenter.setLocationSha1().empty()) {
        reply = (redisReply *)redisCommand(cache, "EVAL %s 1 Location:%d %d %d %d %d", SET_LOCATION_CMD, roleId, lToken, _type, _id, TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(cache, "EVALSHA %s 1 Location:%d %d %d %d %d", g_GameCenter.setLocationSha1().c_str(), roleId, lToken, _type, _id, TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        g_GameCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d set location failed\n", userId, roleId);
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        g_GameCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d set location failed for return type invalid\n", userId, roleId);
        return false;
    }

    if (reply->integer == 0) {
        // 设置失败
        freeReplyObject(reply);
        g_GameCenter.getCachePool()->proxy.put(cache, false);
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d set location failed for already set\n", userId, roleId);
        return false;
    }

    freeReplyObject(reply);
    g_GameCenter.getCachePool()->proxy.put(cache, false);

    // 向Record服发加载数据RPC
    std::string roleData;
    ServerId serverId; // 角色所属区服号
    if (!g_RecordClient.loadRoleData(recordId, roleId, lToken, serverId, roleData)) {
        ERROR_LOG("GameObjectManager::loadRole -- user %d role %d load role data failed\n", userId, roleId);
        return false;
    }

    // 这里是否需要加一次对location超时设置，避免加载数据时location过期？
    // 如果这里不加检测，创建的GameObject会等到第一次心跳设置location时才销毁
    // 如果加检测会让登录流程延长
    // cache = g_GameCenter.getCachePool()->proxy.take();
    // reply = (redisReply *)redisCommand(cache, "EXPIRE Location:%d %d", roleId, 60);
    // if (reply->integer != 1) {
    //     // 设置超时失败，可能是key已经过期
    //     freeReplyObject(reply);
    //     g_GameCenter.getCachePool()->proxy.put(cache, false);
    //     ERROR_LOG("InnerLobbyServiceImpl::initRole -- user %d role %d load role data failed for cant set location expire\n", userId, roleId);
    //     return;
    // }
    // freeReplyObject(reply);
    // g_GameCenter.getCachePool()->proxy.put(cache, false);

    // 创建GameObject
    if (_shutdown) {
        WARN_LOG("GameObjectManager::loadRole -- already shutdown\n");
        return false;
    }

    if (_roleId2GameObjectMap.find(roleId) != _roleId2GameObjectMap.end()) {
        ERROR_LOG("GameObjectManager::loadRole -- game object already exist\n");
        return false;
    }

    if (!g_GameCenter.getCreateGameObjectHandle()) {
        ERROR_LOG("GameObjectManager::loadRole -- not set CreateGameObjectHandle\n");
        return false;
    }

    // 创建GameObject
    auto obj = g_GameCenter.getCreateGameObjectHandle()(userId, roleId, serverId, lToken, this, roleData);

    // 设置gateway和record stub
    if (!obj->setGatewayServerStub(gatewayId)) {
        ERROR_LOG("GameObjectManager::loadRole -- can't set gateway stub\n");
        return false;
    }

    if (!obj->setRecordServerStub(recordId)) {
        ERROR_LOG("GameObjectManager::loadRole -- can't set record stub\n");
        return false;
    }

    _roleId2GameObjectMap.insert(std::make_pair(roleId, obj));
    obj->start();

    return true;
}

bool GameObjectManager::remove(RoleId roleId) {
    auto it = _roleId2GameObjectMap.find(roleId);
    if (it == _roleId2GameObjectMap.end()) {
        return false;
    }

    it->second->stop();
    _roleId2GameObjectMap.erase(it);

    return true;
}
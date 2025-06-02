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

#include "lobby_object_manager.h"
#include "corpc_routine_env.h"
#include "corpc_pubsub.h"
#include "lobby_config.h"
//#include "lobby_delegate.h"
#include "redis_pool.h"
#include "redis_utils.h"
//#include "game_delegate.h"
//#include "game_center.h"
//#include "client_center.h"
#include "demo_lobby_object_data.h"
#include "message_handle_manager.h"
#include "agent_manager.h"
#include "record_agent.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

void LobbyObjectManager::init() {
}

void LobbyObjectManager::shutdown() {
    if (shutdown_) {
        return;
    }

    shutdown_ = true;
    
    for (auto &lobbyObj : roleId2LobbyObjectMap_) {
        lobbyObj.second->stop();
    }

    roleId2LobbyObjectMap_.clear();
}

bool LobbyObjectManager::existRole(RoleId roleId) {
    return roleId2LobbyObjectMap_.find(roleId) != roleId2LobbyObjectMap_.end();
}

std::shared_ptr<LobbyObject> LobbyObjectManager::getLobbyObject(RoleId roleId) {
    auto it = roleId2LobbyObjectMap_.find(roleId);
    if (it == roleId2LobbyObjectMap_.end()) {
        return nullptr;
    }

    return it->second;
}

bool LobbyObjectManager::loadRole(RoleId roleId, ServerId gatewayId) {
    // 判断LobbyObject是否已经存在
    if (existRole(roleId)) {
        ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] game object already exist\n", roleId);
        return false;
    }

    // 查询记录对象位置
    //   若记录对象不存在，分配（负载均衡）record服
    // 尝试设置location
    // 设置成功时加载玩家数据（附带创建记录对象）
    // 重新设置location过期（若这里不设置，后面创建的LobbyObject会等到第一次心跳时才销毁）
    // 创建LobbyObject
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] connect to cache failed\n", roleId);
        return false;
    }

    ServerId recordId;
    RedisAccessResult res = RedisUtils::GetRecordAddress(cache, roleId, recordId);
    switch (res) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] get record failed for db error\n", roleId);
            return false;
        }
        case REDIS_FAIL: {
            RecordAgent *recordAgent = (RecordAgent*)g_AgentManager.getAgent(SERVER_TYPE_RECORD);
            if (!recordAgent->randomServer(recordId)) {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] random record server failed\n", roleId);
                return false;
            }
        }
    }

    // 生成lToken（直接用当前时间来生成）
    struct timeval t;
    gettimeofday(&t, NULL);
    std::string lToken = std::to_string((t.tv_sec % 1000) * 1000000 + t.tv_usec);

    res = RedisUtils::SetLobbyAddress(cache, roleId, g_LobbyConfig.getId(), lToken);
    switch (res) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] set location failed\n", roleId);
            return false;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] set location failed for already set\n", roleId);
            return false;
        }
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 向Record服发加载数据RPC
    std::string roleData;
    ServerId serverId; // 角色所属区服号
    UserId userId;
    RecordAgent *recordAgent = (RecordAgent*)g_AgentManager.getAgent(SERVER_TYPE_RECORD);
    if (!recordAgent->loadRoleData(recordId, roleId, userId, lToken, serverId, roleData)) {
        ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] load role data failed\n", roleId);
        return false;
    }
    DEBUG_LOG("LobbyObjectManager::loadRole -- load from record, userId:%d\n", userId);

    // 这里是否需要加一次对location超时设置，避免加载数据时location过期？
    // 如果这里不加检测，创建的LobbyObject会等到第一次心跳设置location时才销毁
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

    // 创建LobbyObject
    if (shutdown_) {
        WARN_LOG("LobbyObjectManager::loadRole -- user[%llu] role[%llu] already shutdown\n", userId, roleId);
        return false;
    }

    if (roleId2LobbyObjectMap_.find(roleId) != roleId2LobbyObjectMap_.end()) {
        ERROR_LOG("LobbyObjectManager::loadRole -- user[%llu] role[%llu] game object already exist\n", userId, roleId);
        return false;
    }

    //if (!g_LobbyDelegate.getCreateLobbyObjectHandle()) {
    //    ERROR_LOG("LobbyObjectManager::loadRole -- user[%llu] role[%llu] not set CreateLobbyObjectHandle\n", userId, roleId);
    //    return false;
    //}
    //
    //// 创建LobbyObject
    //auto obj = g_LobbyDelegate.getCreateLobbyObjectHandle()(userId, roleId, serverId, lToken, roleData);
    
    auto obj = createLobbyObject(userId, roleId, serverId, lToken, roleData);
    if (!obj) {
        ERROR_LOG("LobbyObjectManager::loadRole -- user[%llu] role[%llu] create game object failed\n", userId, roleId);
        return false;
    }

    // 设置gateway和record stub
    if (gatewayId == 0) {
        // 先尝试查找玩家的gatewayId
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] connect to cache failed when get session\n", roleId);
            return false;
        }

        ServerId gwid = 0;
        std::string gToken;
        RoleId rid = 0;
        switch (RedisUtils::GetSession(cache, userId, gwid, gToken, rid)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] get session failed\n", roleId);
                return false;
            }
            case REDIS_FAIL: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("LobbyObjectManager::loadRole -- role[%llu] get session failed for return type invalid\n", roleId);
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

    // 允许gatewayId为0（允许离线加载角色）
    obj->setGatewayServerId(gatewayId);

    obj->setRecordServerId(recordId);

    roleId2LobbyObjectMap_.insert(std::make_pair(roleId, obj));
    obj->start();

    return true;
}

void LobbyObjectManager::leaveGame(RoleId roleId) {
    auto it = roleId2LobbyObjectMap_.find(roleId);
    assert(it != roleId2LobbyObjectMap_.end());

    it->second->stop();
    roleId2LobbyObjectMap_.erase(it);
}

std::shared_ptr<LobbyObject> LobbyObjectManager::createLobbyObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &ltoken, const std::string &data) {
    std::shared_ptr<LobbyObject> obj(new LobbyObject(userId, roleId, serverId, ltoken));

    std::unique_ptr<LobbyObjectData> objData(new demo::DemoLobbyObjectData());

    if (!objData->initData(data)) {
        ERROR_LOG("create lobby object failed because init data failed, role: %d\n", roleId);
        return nullptr;
    }

    obj->setObjectData(std::move(objData));

    return obj;
}
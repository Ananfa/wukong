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

#include "lobby_service.h"
#include "lobby_config.h"
#include "game_center.h"
#include "share/const.h"

using namespace wukong;

void LobbyServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                const ::corpc::Void* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    _manager->shutdown();
}

void LobbyServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                      const ::corpc::Void* request,
                                      ::wukong::pb::Uint32Value* response,
                                      ::google::protobuf::Closure* done) {
    response->set_value(_manager->size());
}

void LobbyServiceImpl::initRole(::google::protobuf::RpcController* controller,
                                const ::wukong::pb::InitRoleRequest* request,
                                ::wukong::pb::Uint32Value* response,
                                ::google::protobuf::Closure* done) {
    UserId userId = request->userid();
    RoleId roleId = request->roleid();
    ServerId gwId = request->gatewayid();

    // 判断GameObject是否已经存在
    if (_manager->exist(roleId)) {
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d game object already exist\n", userId, roleId);
        return;
    }

    // 查询记录对象位置
    //   若记录对象不存在，分配（负载均衡）record服
    // 尝试设置location
    // 设置成功时加载玩家数据（附带创建记录对象）
    // 重新设置location过期（若这里不设置，后面创建的GameObject会等到第一次心跳时才销毁）
    // 创建GameObject
    redisContext *cache = g_GameCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d connect to cache failed\n", userId, roleId);
        return;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "HGET Record:%d loc", roleId);
    if (!reply) {
        g_GameCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d get record failed for db error\n", userId, roleId);
        return;
    }

    ServerId rcId;
    if (reply->type == REDIS_REPLY_STRING) {
        rcId = std::stoi(reply->str);
    } else if (reply->type == REDIS_REPLY_NIL) {
        if (!g_GameCenter.randomRecordServer(rcId)) {
            freeReplyObject(reply);
            g_GameCenter.getCachePool()->proxy.put(cache, false);
            ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d random record server failed\n", userId, roleId);
            return;
        }
    } else {
        freeReplyObject(reply);
        g_GameCenter.getCachePool()->proxy.put(cache, false);
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d get record failed for invalid data type\n", userId, roleId);
        return;
    }

    freeReplyObject(reply);

    // 生成lToken（直接用当前时间来生成）
    struct timeval t;
    gettimeofday(&t, NULL);
    uint32_t lToken = (t.tv_sec % 1000) * 1000000 + t.tv_usec;

    // 尝试设置location
    // TODO: loc的格式应该包含游戏服类型
    if (g_GameCenter.setLocationSha1().empty()) {
        reply = (redisReply *)redisCommand(cache, "EVAL %s 1 Location:%d %d %d %d %d", SET_LOCATION_CMD, roleId, lToken, GAME_SERVER_TYPE_LOBBY, _manager->getId(), TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(cache, "EVALSHA %s 1 Location:%d %d %d %d %d", g_GameCenter.setLocationSha1().c_str(), roleId, lToken, GAME_SERVER_TYPE_LOBBY, _manager->getId(), TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        g_GameCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d set location failed\n", userId, roleId);
        return;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        g_GameCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d set location failed for return type invalid\n", userId, roleId);
        return;
    }

    if (reply->integer == 0) {
        // 设置失败
        freeReplyObject(reply);
        g_GameCenter.getCachePool()->proxy.put(cache, false);
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d set location failed for already set\n", userId, roleId);
        return;
    }

    freeReplyObject(reply);
    g_GameCenter.getCachePool()->proxy.put(cache, false);

    // 向Record服发加载数据RPC
    std::string roleData;
    ServerId serverId; // 角色所属区服号
    if (!g_RecordClient.loadRole(rcId, roleId, lToken, serverId, roleData)) {
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d load role data failed\n", userId, roleId);
        return;
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
    //     ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d load role data failed for cant set location expire\n", userId, roleId);
    //     return;
    // }
    // freeReplyObject(reply);
    // g_GameCenter.getCachePool()->proxy.put(cache, false);

    // 创建GameObject
    if (!_manager->create(userId, roleId, serverId, lToken, gwId, rcId, roleData)) {
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d create game object failed\n", userId, roleId);
        return;
    }

    response->set_value(lToken);
}

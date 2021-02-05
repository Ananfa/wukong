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
    // TODO:
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

    // TODO: 
    // 先加载角色数据
    //   查询记录对象位置
    //   若记录对象不存在，分配（负载均衡）record服
    //   加载玩家数据（附带创建记录对象）
    // 尝试设置location（在这里才设置location是避免在加载数据前设置可能导致location过期）
    // 创建GameObject
    redisContext *cache = g_GameCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d connect to cache failed\n", userId, roleId);
        return;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "HGET record:%d loc", roleId);
    if (!reply) {
        g_GameCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d get record failed for db error\n", userId, roleId);
        return;
    }

    ServerId rdId;
    if (reply->type == REDIS_REPLY_STRING) {
        rdId = std::stoi(reply->str);
    } else if (reply->type == REDIS_REPLY_NIL) {
        if (!g_GameCenter.randomRecordServer(rdId)) {
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

    // 生成lToken（直接用当前时间）
    struct timeval t;
    gettimeofday(&t, NULL);
    uint32_t lToken = t.tv_sec;

    // 尝试设置location
    if (g_GameCenter.setLocationSha1().empty()) {
        reply = (redisReply *)redisCommand(cache, "eval %s 1 location:%d %s %d %d", SET_LOCATION_CMD, roleId, lToken, _manager->getId(), 60);
    } else {
        reply = (redisReply *)redisCommand(cache, "evalsha %s 1 location:%d %s %d %d %d", g_GameCenter.setLocationSha1().c_str(), roleId, lToken, _manager->getId(), 60);
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
    if (!g_RecordClient.loadRole(rdId, roleId, lToken, roleData)) {
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d load role data failed\n", userId, roleId);
        return;
    }

    // 这里是否需要加一次对location超时设置，避免加载数据时location过期？
    // 如果这里不加检测，创建的GameObject会等到第一次心跳设置location时才销毁

    // 创建GameObject
    if (!_manager->create(userId, roleId, lToken, gwId, rdId, roleData)) {
        ERROR_LOG("LobbyServiceImpl::initRole -- user %d role %d create game object failed\n", userId, roleId);
        return;
    }

    response->set_value(lToken);
}

/*
 * Created by Xianke Liu on 2021/5/6.
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

#include "record_service.h"
#include "record_config.h"
#include "record_server.h"
#include "redis_pool.h"
#include "mysql_pool.h"
#include "proto_utils.h"
#include "redis_utils.h"
#include "mysql_utils.h"
#include "share/const.h"

using namespace wukong;

void RecordServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                              const ::corpc::Void* request,
                              ::corpc::Void* response,
                              ::google::protobuf::Closure* done) {
    g_RecordObjectManager.shutdown();
}

void RecordServiceImpl::loadRoleData(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LoadRoleDataRequest* request,
                             ::wukong::pb::LoadRoleDataResponse* response,
                             ::google::protobuf::Closure* done) {
    RoleId roleId = request->roleid();

    if (g_RecordObjectManager.isShutdown()) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] server is shutdown\n", roleId);
        response->set_errcode(1);
        return;
    }

    std::list<std::pair<std::string, std::string>> datas;

    // 查看是否有本地缓存的record object
    std::shared_ptr<RecordObject> obj = g_RecordObjectManager.getRecordObject(roleId);
    if (obj) {
        obj->setLToken(request->ltoken());

        response->set_userid(obj->getUserId());
        response->set_serverid(obj->getServerId());

        obj->buildAllDatas(datas);
        response->set_data(ProtoUtils::marshalDataFragments(datas));
        return;
    }
    
    // 设置record
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] connect to cache failed\n", roleId);
        response->set_errcode(2);
        return;
    }

    // 生成rToken（直接用当前时间）
    struct timeval t;
    gettimeofday(&t, NULL);
    std::string rToken = std::to_string(t.tv_sec);

    switch (RedisUtils::SetRecordAddress(cache, roleId, g_RecordConfig.getId(), rToken)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] set record failed\n", roleId);
            response->set_errcode(3);
            return;
        }
        case REDIS_FAIL: {
            // 设置失败
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] set record failed for already set\n", roleId);
            response->set_errcode(5);
            return;
        }
    }

    // 加载玩家数据
    // 先从redis加载玩家数据，若redis没有，则从mysql加载并缓存到redis中
    UserId userId;
    ServerId serverId;
    if (RedisUtils::LoadRole(cache, roleId, userId, serverId, datas, true) == REDIS_DB_ERROR) {
        g_RedisPoolManager.getCoreCache()->put(cache, true);
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] load failed\n", roleId);
        response->set_errcode(6);
        return;
    }

    if (datas.size() > 0) {
        g_RedisPoolManager.getCoreCache()->put(cache, false);

        // 创建record object
        obj = g_RecordObjectManager.create(userId, roleId, serverId, rToken, datas);
        if (!obj) {
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] create record object failed\n", roleId);
            response->set_errcode(7);
            return;
        }

        obj->setLToken(request->ltoken());

        response->set_userid(userId);
        response->set_serverid(serverId);
        response->set_data(ProtoUtils::marshalDataFragments(datas));

        return;
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 从MySQL加载角色数据并缓存到redis中
    MYSQL *mysql = g_MysqlPoolManager.getCoreRecord()->take();
    if (!mysql) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] connect to mysql failed\n", roleId);
        response->set_errcode(8);
        return;
    }

    std::string data;
    if (!MysqlUtils::LoadRole(mysql, roleId, userId, serverId, data)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] load role from mysql failed\n", roleId);
        response->set_errcode(9);
        return;
    }
    DEBUG_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu]\n", userId, roleId);

    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);

    if (data.empty()) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] no role data\n", userId, roleId);
        response->set_errcode(10);
        return;
    }

    if (!ProtoUtils::unmarshalDataFragments(data, datas)) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] unmarshal role data failed\n", userId, roleId);
        response->set_errcode(11);
        return;
    }

    if (datas.size() == 0) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] role data is empty\n", userId, roleId);
        response->set_errcode(12);
        return;
    }

    // 缓存到redis中
    cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] can't cache role data for connect to cache failed\n", userId, roleId);
        response->set_errcode(13);
        return;
    }

    switch (RedisUtils::SaveRole(cache, roleId, userId, serverId, datas)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] cache role data failed for db error\n", userId, roleId);
            response->set_errcode(13);
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] cache role data failed\n", userId, roleId);
            response->set_errcode(14);
            return;
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 创建record object
    obj = g_RecordObjectManager.create(userId, roleId, serverId, rToken, datas);
    if (!obj) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] create record object failed\n", userId, roleId);
        response->set_errcode(7);
        return;
    }

    obj->setLToken(request->ltoken());

    // 返回角色数据
    response->set_userid(userId);
    response->set_serverid(serverId);
    response->set_data(ProtoUtils::marshalDataFragments(datas));
}

void RecordServiceImpl::sync(::google::protobuf::RpcController* controller,
                          const ::wukong::pb::SyncRequest* request,
                          ::wukong::pb::BoolValue* response,
                          ::google::protobuf::Closure* done) {
    // 数据同步
    std::shared_ptr<RecordObject> obj = g_RecordObjectManager.getRecordObject(request->roleid());
    if (!obj) {
        ERROR_LOG("InnerRecordServiceImpl::sync -- role[%llu] record object not found\n", request->roleid());
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("InnerRecordServiceImpl::sync -- role[%llu] ltoken not match\n", request->roleid());
        return;
    }

    obj->syncIn(request);
    response->set_value(true);
}

void RecordServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::RSHeartbeatRequest* request,
                               ::wukong::pb::BoolValue* response,
                               ::google::protobuf::Closure* done) {
    std::shared_ptr<RecordObject> obj = g_RecordObjectManager.getRecordObject(request->roleid());
    if (!obj) {
        WARN_LOG("InnerRecordServiceImpl::heartbeat -- role[%llu] record object not found\n", request->roleid());
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("InnerRecordServiceImpl::heartbeat -- role[%llu] ltoken not match\n", request->roleid());
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    // 注意：对于记录对象来说，当游戏对象销毁后记录对象可以缓存久一些，减轻数据库压力，这里设置10分钟
    obj->gameObjectHeartbeatExpire_ = t.tv_sec + RECORD_TIMEOUT;
    response->set_value(true);
}

//void RecordServiceImpl::addInnerStub(ServerId sid, pb::InnerRecordService_Stub* stub) {
//    innerStubs_.insert(std::make_pair(sid, stub));
//}
//
//pb::InnerRecordService_Stub *RecordServiceImpl::getInnerStub(ServerId sid) {
//    auto it = innerStubs_.find(sid);
//
//    if (it == innerStubs_.end()) {
//        return nullptr;
//    }
//
//    return it->second;
//}
//
//void RecordServiceImpl::traverseInnerStubs(std::function<bool(ServerId, pb::InnerRecordService_Stub*)> handle) {
//    for (auto &pair : innerStubs_) {
//        if (!handle(pair.first, pair.second)) {
//            return;
//        }
//    }
//}
//
//void InnerRecordServiceImpl::shutdown(::google::protobuf::RpcController* controller,
//                                const ::corpc::Void* request,
//                                ::corpc::Void* response,
//                                ::google::protobuf::Closure* done) {
//    manager_->shutdown();
//}
//
//void InnerRecordServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
//                                      const ::corpc::Void* request,
//                                      ::wukong::pb::Uint32Value* response,
//                                      ::google::protobuf::Closure* done) {
//    response->set_value(manager_->size());
//}
//
//void InnerRecordServiceImpl::loadRoleData(::google::protobuf::RpcController* controller,
//                                 const ::wukong::pb::LoadRoleDataRequest* request,
//                                 ::wukong::pb::LoadRoleDataResponse* response,
//                                 ::google::protobuf::Closure* done) {
//    RoleId roleId = request->roleid();
//
//    if (manager_->isShutdown()) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] server is shutdown\n", roleId);
//        response->set_errcode(1);
//        return;
//    }
//
//    std::list<std::pair<std::string, std::string>> datas;
//
//    // 查看是否有本地缓存的record object
//    std::shared_ptr<RecordObject> obj = manager_->getRecordObject(roleId);
//    if (obj) {
//        obj->setLToken(request->ltoken());
//
//        response->set_userid(obj->getUserId());
//        response->set_serverid(obj->getServerId());
//
//        obj->buildAllDatas(datas);
//        response->set_data(ProtoUtils::marshalDataFragments(datas));
//        return;
//    }
//    
//    // 设置record
//    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
//    if (!cache) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] connect to cache failed\n", roleId);
//        response->set_errcode(2);
//        return;
//    }
//
//    // 生成rToken（直接用当前时间）
//    struct timeval t;
//    gettimeofday(&t, NULL);
//    std::string rToken = std::to_string(t.tv_sec);
//
//    switch (RedisUtils::SetRecordAddress(cache, roleId, manager_->getId(), rToken)) {
//        case REDIS_DB_ERROR: {
//            g_RedisPoolManager.getCoreCache()->put(cache, true);
//            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] set record failed\n", roleId);
//            response->set_errcode(3);
//            return;
//        }
//        case REDIS_FAIL: {
//            // 设置失败
//            g_RedisPoolManager.getCoreCache()->put(cache, false);
//            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] set record failed for already set\n", roleId);
//            response->set_errcode(5);
//            return;
//        }
//    }
//
//    // 加载玩家数据
//    // 先从redis加载玩家数据，若redis没有，则从mysql加载并缓存到redis中
//    UserId userId;
//    ServerId serverId;
//    if (RedisUtils::LoadRole(cache, roleId, userId, serverId, datas, true) == REDIS_DB_ERROR) {
//        g_RedisPoolManager.getCoreCache()->put(cache, true);
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] load failed\n", roleId);
//        response->set_errcode(6);
//        return;
//    }
//
//    if (datas.size() > 0) {
//        g_RedisPoolManager.getCoreCache()->put(cache, false);
//
//        // 创建record object
//        obj = manager_->create(userId, roleId, serverId, rToken, datas);
//        if (!obj) {
//            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] create record object failed\n", roleId);
//            response->set_errcode(7);
//            return;
//        }
//
//        obj->setLToken(request->ltoken());
//
//        response->set_userid(userId);
//        response->set_serverid(serverId);
//        response->set_data(ProtoUtils::marshalDataFragments(datas));
//
//        return;
//    }
//
//    g_RedisPoolManager.getCoreCache()->put(cache, false);
//
//    // 从MySQL加载角色数据并缓存到redis中
//    MYSQL *mysql = g_MysqlPoolManager.getCoreRecord()->take();
//    if (!mysql) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] connect to mysql failed\n", roleId);
//        response->set_errcode(8);
//        return;
//    }
//
//    std::string data;
//    if (!MysqlUtils::LoadRole(mysql, roleId, userId, serverId, data)) {
//        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- role[%llu] load role from mysql failed\n", roleId);
//        response->set_errcode(9);
//        return;
//    }
//    DEBUG_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu]\n", userId, roleId);
//
//    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);
//
//    if (data.empty()) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] no role data\n", userId, roleId);
//        response->set_errcode(10);
//        return;
//    }
//
//    if (!ProtoUtils::unmarshalDataFragments(data, datas)) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] unmarshal role data failed\n", userId, roleId);
//        response->set_errcode(11);
//        return;
//    }
//
//    if (datas.size() == 0) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] role data is empty\n", userId, roleId);
//        response->set_errcode(12);
//        return;
//    }
//
//    // 缓存到redis中
//    cache = g_RedisPoolManager.getCoreCache()->take();
//    if (!cache) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] can't cache role data for connect to cache failed\n", userId, roleId);
//        response->set_errcode(13);
//        return;
//    }
//
//    switch (RedisUtils::SaveRole(cache, roleId, userId, serverId, datas)) {
//        case REDIS_DB_ERROR: {
//            g_RedisPoolManager.getCoreCache()->put(cache, true);
//            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] cache role data failed for db error\n", userId, roleId);
//            response->set_errcode(13);
//            return;
//        }
//        case REDIS_FAIL: {
//            g_RedisPoolManager.getCoreCache()->put(cache, false);
//            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] cache role data failed\n", userId, roleId);
//            response->set_errcode(14);
//            return;
//        }
//    }
//    g_RedisPoolManager.getCoreCache()->put(cache, false);
//
//    // 创建record object
//    obj = manager_->create(userId, roleId, serverId, rToken, datas);
//    if (!obj) {
//        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- user[%llu] role[%llu] create record object failed\n", userId, roleId);
//        response->set_errcode(7);
//        return;
//    }
//
//    obj->setLToken(request->ltoken());
//
//    // 返回角色数据
//    response->set_userid(userId);
//    response->set_serverid(serverId);
//    response->set_data(ProtoUtils::marshalDataFragments(datas));
//}
//
//void InnerRecordServiceImpl::sync(::google::protobuf::RpcController* controller,
//                             const ::wukong::pb::SyncRequest* request,
//                             ::wukong::pb::BoolValue* response,
//                             ::google::protobuf::Closure* done) {
//    // 数据同步
//    std::shared_ptr<RecordObject> obj = manager_->getRecordObject(request->roleid());
//    if (!obj) {
//        ERROR_LOG("InnerRecordServiceImpl::sync -- role[%llu] record object not found\n", request->roleid());
//        return;
//    }
//
//    if (obj->getLToken() != request->ltoken()) {
//        ERROR_LOG("InnerRecordServiceImpl::sync -- role[%llu] ltoken not match\n", request->roleid());
//        return;
//    }
//
//    obj->syncIn(request);
//    response->set_value(true);
//}
//
//void InnerRecordServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
//                                  const ::wukong::pb::RSHeartbeatRequest* request,
//                                  ::wukong::pb::BoolValue* response,
//                                  ::google::protobuf::Closure* done) {
//    std::shared_ptr<RecordObject> obj = manager_->getRecordObject(request->roleid());
//    if (!obj) {
//        WARN_LOG("InnerRecordServiceImpl::heartbeat -- role[%llu] record object not found\n", request->roleid());
//        return;
//    }
//
//    if (obj->getLToken() != request->ltoken()) {
//        ERROR_LOG("InnerRecordServiceImpl::heartbeat -- role[%llu] ltoken not match\n", request->roleid());
//        return;
//    }
//
//    struct timeval t;
//    gettimeofday(&t, NULL);
//    // 注意：对于记录对象来说，当游戏对象销毁后记录对象可以缓存久一些，减轻数据库压力，这里设置10分钟
//    obj->gameObjectHeartbeatExpire_ = t.tv_sec + RECORD_TIMEOUT;
//    response->set_value(true);
//}

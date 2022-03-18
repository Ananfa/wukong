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
    traverseInnerStubs([](ServerId sid, pb::InnerRecordService_Stub *stub) -> bool {
        stub->shutdown(NULL, NULL, NULL, NULL);
        return true;
    });
}

void RecordServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                    const ::corpc::Void* request,
                                    ::wukong::pb::OnlineCounts* response,
                                    ::google::protobuf::Closure* done) {
    // 注意：这里处理中间会产生协程切换，遍历的Map可能在过程中被修改，因此traverseInnerStubs方法中对map进行了镜像复制
    traverseInnerStubs([request, response](ServerId sid, pb::InnerRecordService_Stub *stub) -> bool {
        pb::Uint32Value *resp = new pb::Uint32Value();
        corpc::Controller *ctl = new corpc::Controller();
        stub->getOnlineCount(ctl, request, resp, NULL);
        pb::OnlineCount *onlineCount = response->add_counts();
        onlineCount->set_serverid(sid);
        if (ctl->Failed()) {
            onlineCount->set_count(-1);
        } else {
            onlineCount->set_count(resp->value());
        }
        delete ctl;
        delete resp;

        return true;
    });
}

void RecordServiceImpl::loadRoleData(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LoadRoleDataRequest* request,
                             ::wukong::pb::LoadRoleDataResponse* response,
                             ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->loadRoleData(controller, request, response, done);
}

void RecordServiceImpl::sync(::google::protobuf::RpcController* controller,
                          const ::wukong::pb::SyncRequest* request,
                          ::wukong::pb::BoolValue* response,
                          ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->sync(controller, request, response, done);
}

void RecordServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::RSHeartbeatRequest* request,
                               ::wukong::pb::BoolValue* response,
                               ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->heartbeat(controller, request, response, done);
}

void RecordServiceImpl::addInnerStub(ServerId sid, pb::InnerRecordService_Stub* stub) {
    _innerStubs.insert(std::make_pair(sid, stub));
}

pb::InnerRecordService_Stub *RecordServiceImpl::getInnerStub(ServerId sid) {
    auto it = _innerStubs.find(sid);

    if (it == _innerStubs.end()) {
        return nullptr;
    }

    return it->second;
}

void RecordServiceImpl::traverseInnerStubs(std::function<bool(ServerId, pb::InnerRecordService_Stub*)> handle) {
    for (auto &pair : _innerStubs) {
        if (!handle(pair.first, pair.second)) {
            return;
        }
    }
}

void InnerRecordServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                const ::corpc::Void* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    _manager->shutdown();
}

void InnerRecordServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                      const ::corpc::Void* request,
                                      ::wukong::pb::Uint32Value* response,
                                      ::google::protobuf::Closure* done) {
    response->set_value(_manager->size());
}

void InnerRecordServiceImpl::loadRoleData(::google::protobuf::RpcController* controller,
                                 const ::wukong::pb::LoadRoleDataRequest* request,
                                 ::wukong::pb::LoadRoleDataResponse* response,
                                 ::google::protobuf::Closure* done) {
    uint32_t roleId = request->roleid();

    if (_manager->isShutdown()) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] server is shutdown\n", roleId);
        response->set_errcode(1);
        return;
    }

    std::list<std::pair<std::string, std::string>> datas;

    // 查看是否有本地缓存的record object
    std::shared_ptr<RecordObject> obj = _manager->getRecordObject(roleId);
    if (obj) {
        obj->setLToken(request->ltoken());

        response->set_serverid(obj->getServerId());

        obj->buildAllDatas(datas);
        response->set_data(ProtoUtils::marshalDataFragments(datas));
        return;
    }
    
    // 设置record
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] connect to cache failed\n", roleId);
        response->set_errcode(2);
        return;
    }

    // 生成rToken（直接用当前时间）
    struct timeval t;
    gettimeofday(&t, NULL);
    std::string rToken = std::to_string(t.tv_sec);

    switch (RedisUtils::SetRecordAddress(cache, roleId, _manager->getId(), rToken)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] set record failed\n", roleId);
            response->set_errcode(3);
            return;
        }
        case REDIS_FAIL: {
            // 设置失败
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] set record failed for already set\n", roleId);
            response->set_errcode(5);
            return;
        }
    }

    // 加载玩家数据
    // 先从redis加载玩家数据，若redis没有，则从mysql加载并缓存到redis中
    ServerId serverId;
    if (RedisUtils::LoadRole(cache, roleId, serverId, datas, true) == REDIS_DB_ERROR) {
        g_RedisPoolManager.getCoreCache()->put(cache, true);
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] load failed\n", roleId);
        response->set_errcode(6);
        return;
    }

    if (datas.size() > 0) {
        g_RedisPoolManager.getCoreCache()->put(cache, false);

        // 创建record object
        obj = _manager->create(roleId, serverId, rToken, datas);
        if (!obj) {
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] create record object failed\n", roleId);
            response->set_errcode(7);
            return;
        }

        obj->setLToken(request->ltoken());

        response->set_serverid(serverId);
        response->set_data(ProtoUtils::marshalDataFragments(datas));

        return;
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 从MySQL加载角色数据并缓存到redis中
    MYSQL *mysql = g_MysqlPoolManager.getCoreRecord()->take();
    if (!mysql) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] connect to mysql failed\n", roleId);
        response->set_errcode(8);
        return;
    }

    std::string data;
    if (!MysqlUtils::LoadRole(mysql, roleId, serverId, data)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] load role from mysql failed\n", roleId);
        response->set_errcode(9);
        return;
    }

    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);

    if (data.empty()) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] no role data\n");
        response->set_errcode(10);
        return;
    }

    if (!ProtoUtils::unmarshalDataFragments(data, datas)) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] unmarshal role data failed\n");
        response->set_errcode(11);
        return;
    }

    if (datas.size() == 0) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] role data is empty\n");
        response->set_errcode(12);
        return;
    }

    // 缓存到redis中
    cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] can't cache role data for connect to cache failed\n", roleId);
        response->set_errcode(13);
        return;
    }

    switch (RedisUtils::SaveRole(cache, roleId, serverId, datas)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] cache role data failed for db error\n", roleId);
            response->set_errcode(13);
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] cache role data failed\n", roleId);
            response->set_errcode(14);
            return;
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 创建record object
    obj = _manager->create(roleId, serverId, rToken, datas);
    if (!obj) {
        ERROR_LOG("InnerRecordServiceImpl::loadRoleData -- [role %d] create record object failed\n", roleId);
        response->set_errcode(7);
        return;
    }

    obj->setLToken(request->ltoken());

    // 返回角色数据
    response->set_serverid(serverId);
    response->set_data(ProtoUtils::marshalDataFragments(datas));
}

void InnerRecordServiceImpl::sync(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::SyncRequest* request,
                             ::wukong::pb::BoolValue* response,
                             ::google::protobuf::Closure* done) {
    // 数据同步
    std::shared_ptr<RecordObject> obj = _manager->getRecordObject(request->roleid());
    if (!obj) {
        ERROR_LOG("InnerRecordServiceImpl::sync -- record object not found\n");
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("InnerRecordServiceImpl::sync -- ltoken not match\n");
        return;
    }

    obj->syncIn(request);
    response->set_value(true);
}

void InnerRecordServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                                  const ::wukong::pb::RSHeartbeatRequest* request,
                                  ::wukong::pb::BoolValue* response,
                                  ::google::protobuf::Closure* done) {
    std::shared_ptr<RecordObject> obj = _manager->getRecordObject(request->roleid());
    if (!obj) {
        ERROR_LOG("InnerRecordServiceImpl::heartbeat -- record object not found\n");
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("InnerRecordServiceImpl::heartbeat -- ltoken not match\n");
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    // 注意：对于记录对象来说，当游戏对象销毁后记录对象可以缓存久一些，减轻数据库压力，这里设置10分钟
    obj->_gameObjectHeartbeatExpire = t.tv_sec + RECORD_TIMEOUT;
    response->set_value(true);
}

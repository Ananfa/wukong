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
#include "record_center.h"
#include "proto_utils.h"
#include "redis_utils.h"
#include "mysql_utils.h"
#include "share/const.h"

using namespace wukong;

void RecordServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                const ::corpc::Void* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    _manager->shutdown();
}

void RecordServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                      const ::corpc::Void* request,
                                      ::wukong::pb::Uint32Value* response,
                                      ::google::protobuf::Closure* done) {
    response->set_value(_manager->size());
}

void RecordServiceImpl::loadRole(::google::protobuf::RpcController* controller,
                                 const ::wukong::pb::LoadRoleRequest* request,
                                 ::wukong::pb::LoadRoleResponse* response,
                                 ::google::protobuf::Closure* done) {
    uint32_t roleId = request->roleid();

    if (_manager->isShutdown()) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] server is shutdown\n", roleId);
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
    redisContext *cache = g_RecordCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] connect to cache failed\n", roleId);
        response->set_errcode(2);
        return;
    }

    // 生成rToken（直接用当前时间）
    struct timeval t;
    gettimeofday(&t, NULL);
    uint32_t rToken = t.tv_sec;

    redisReply *reply;
    // 尝试设置record
    if (g_RecordCenter.setRecordSha1().empty()) {
        reply = (redisReply *)redisCommand(cache, "EVAL %s 1 record:%d %s %d", SET_RECORD_CMD, roleId, rToken, 60);
    } else {
        reply = (redisReply *)redisCommand(cache, "EVALSHA %s 1 record:%d %s %d", g_RecordCenter.setRecordSha1().c_str(), roleId, rToken, 60);
    }
    
    if (!reply) {
        g_RecordCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] set record failed\n", roleId);
        response->set_errcode(3);
        return;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        g_RecordCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] set record failed for return type invalid\n", roleId);
        response->set_errcode(4);
        return;
    }

    if (reply->integer == 0) {
        // 设置失败
        freeReplyObject(reply);
        g_RecordCenter.getCachePool()->proxy.put(cache, false);
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] set record failed for already set\n", roleId);
        response->set_errcode(5);
        return;
    }

    freeReplyObject(reply);

    // 加载玩家数据
    // 先从redis加载玩家数据，若redis没有，则从mysql加载并缓存到redis中
    ServerId serverId;
    if (!RedisUtils::LoadRole(cache, g_RecordCenter.loadRoleSha1(), roleId, serverId, datas, true)) {
        g_RecordCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] load failed\n", roleId);
        response->set_errcode(6);
        return;
    }

    if (datas.size() > 0) {
        g_RecordCenter.getCachePool()->proxy.put(cache, false);

        // 创建record object
        obj = _manager->create(roleId, serverId, rToken, datas);
        if (!obj) {
            ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] create record object failed\n", roleId);
            response->set_errcode(7);
            return;
        }

        obj->setLToken(request->ltoken());

        response->set_serverid(serverId);
        response->set_data(ProtoUtils::marshalDataFragments(datas));

        return;
    }

    g_RecordCenter.getCachePool()->proxy.put(cache, false);

    // 从MySQL加载角色数据并缓存到redis中
    MYSQL *mysql = g_RecordCenter.getMysqlPool()->proxy.take();
    if (!mysql) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] connect to mysql failed\n", roleId);
        response->set_errcode(8);
        return;
    }

    std::string data;
    if (!MysqlUtils::LoadRole(mysql, roleId, serverId, data)) {
        g_RecordCenter.getMysqlPool()->proxy.put(mysql, true);
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] load role from mysql failed\n", roleId);
        response->set_errcode(9);
        return;
    }

    g_RecordCenter.getMysqlPool()->proxy.put(mysql, false);

    if (data.empty()) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] no role data\n");
        response->set_errcode(10);
        return;
    }

    if (!ProtoUtils::unmarshalDataFragments(data, datas)) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] unmarshal role data failed\n");
        response->set_errcode(11);
        return;
    }

    if (datas.size() == 0) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] role data is empty\n");
        response->set_errcode(12);
        return;
    }

    // 缓存到redis中
    cache = g_RecordCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] can't cache role data for connect to cache failed\n", roleId);
        response->set_errcode(13);
        return;
    }

    if (!RedisUtils::SaveRole(cache, g_RecordCenter.saveRoleSha1(), roleId, serverId, datas)) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] cache role data failed\n", roleId);
        response->set_errcode(13);
        return;
    }

    // 创建record object
    obj = _manager->create(roleId, serverId, rToken, datas);
    if (!obj) {
        ERROR_LOG("RecordServiceImpl::loadRole -- [role %d] create record object failed\n", roleId);
        response->set_errcode(7);
        return;
    }

    obj->setLToken(request->ltoken());

    // 返回角色数据
    response->set_serverid(serverId);
    response->set_data(ProtoUtils::marshalDataFragments(datas));
}

void RecordServiceImpl::sync(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::SyncRequest* request,
                             ::wukong::pb::BoolValue* response,
                             ::google::protobuf::Closure* done) {
    // 数据同步
    std::shared_ptr<RecordObject> obj = _manager->getRecordObject(request->roleid());
    if (!obj) {
        ERROR_LOG("RecordServiceImpl::sync -- record object not found\n");
        return;
    }

    if (obj->getLToken() == request->ltoken()) {
        ERROR_LOG("RecordServiceImpl::sync -- ltoken not match\n");
        return;
    }

    obj->syncIn(request);
    response->set_value(true);
}

void RecordServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                                  const ::wukong::pb::RSHeartbeatRequest* request,
                                  ::wukong::pb::BoolValue* response,
                                  ::google::protobuf::Closure* done) {
    std::shared_ptr<RecordObject> obj = _manager->getRecordObject(request->roleid());
    if (!obj) {
        ERROR_LOG("RecordServiceImpl::heartbeat -- record object not found\n");
        return;
    }

    if (obj->getLToken() == request->ltoken()) {
        ERROR_LOG("RecordServiceImpl::heartbeat -- ltoken not match\n");
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    // 注意：对于记录对象来说，当游戏对象销毁后记录对象可以缓存久一些，减轻数据库压力，这里设置10分钟
    obj->_gameObjectHeartbeatExpire = t.tv_sec + RECORD_TIMEOUT;
    response->set_value(true);
}

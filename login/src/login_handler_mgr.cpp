/*
 * Created by Xianke Liu on 2020/11/20.
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
#include "corpc_rand.h"
#include "login_handler_mgr.h"
#include "client_center.h"
#include "redis_pool.h"
#include "mysql_pool.h"
#include "json_utils.h"
#include "redis_utils.h"
#include "mysql_utils.h"
#include "proto_utils.h"
#include "login_config.h"
#include "rapidjson/document.h"
#include "share/const.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <random>

using namespace rapidjson;
using namespace corpc;
using namespace wukong;

// 注意：这里是g++编译器的一个bug导致需要用namespace大括号括住模板特化
namespace wukong {
    template<> void JsonWriter::put(const RoleServerInfo &value) {
        writer_.StartObject();
        writer_.Key("s");
        put(value.serverId);
        writer_.Key("r");
        put(value.roleId);
        writer_.EndObject();
    }

    template<> void JsonWriter::put(const RoleProfile &value) {
        writer_.StartObject();
        writer_.Key("serverId");
        put(value.serverId);
        writer_.Key("roleId");
        put(value.roleId);
        if (!value.pData.empty()) {
            writer_.Key("pData");
            put(value.pData);
        }
        writer_.EndObject();
    }
}

std::string LoginHandlerMgr::serverGroupData_;
Mutex LoginHandlerMgr::serverGroupDataLock_;
std::atomic<uint32_t> LoginHandlerMgr::serverGroupDataVersion_(0);
thread_local std::string LoginHandlerMgr::t_serverGroupData_("[]");
thread_local uint32_t LoginHandlerMgr::t_serverGroupDataVersion_(0);
thread_local std::map<GroupId, uint32_t> LoginHandlerMgr::t_groupStatusMap_;
thread_local std::map<ServerId, GroupId> LoginHandlerMgr::t_serverId2groupIdMap_;

void LoginHandlerMgr::init(HttpServer *server) {
    // 获取并订阅服务器组列表信息
    RoutineEnvironment::startCoroutine([](void * arg) -> void* {
        LoginHandlerMgr *self = (LoginHandlerMgr *)arg;
        self->updateServerGroupData();
        return NULL;
    }, this);

    //PubsubService::Subscribe("ServerGroups", true, std::bind(&LoginHandlerMgr::updateServerGroupData, this, std::placeholders::_1, std::placeholders::_2));
    PubsubService::Subscribe("WK_ServerGroups", true, [this](const std::string& topic, const std::string& msg) {
        updateServerGroupData();
    });


    // 注册filter
    // 验证request是否合法
    server->registerFilter([](std::shared_ptr<RequestMessage>& request, std::shared_ptr<ResponseMessage>& response) -> bool {
        // TODO: authentication
//        response->setResult(HttpStatusCode::Unauthorized);
//        response->addHeader("WWW-Authenticate", "Basic realm=\"Need Authentication\"");
        return true;
    });
    
    // 注册handler
    server->registerHandler(POST, "/login", std::bind(&LoginHandlerMgr::login, this, std::placeholders::_1, std::placeholders::_2));

    server->registerHandler(POST, "/getProfile", std::bind(&LoginHandlerMgr::getProfile, this, std::placeholders::_1, std::placeholders::_2));

    server->registerHandler(POST, "/createRole", std::bind(&LoginHandlerMgr::createRole, this, std::placeholders::_1, std::placeholders::_2));

    server->registerHandler(POST, "/enterGame", std::bind(&LoginHandlerMgr::enterGame, this, std::placeholders::_1, std::placeholders::_2));

}

/******************** http api start ********************/

void LoginHandlerMgr::login(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response) {
    DEBUG_LOG("LoginHandlerMgr::login -- start ... \n");
    request->printAll();
    
    if (!request->has("openid")) {
        return setErrorResponse(response, "missing parameter openid");
    }

    std::string openid = (*request)["openid"];

    redisContext *cache = nullptr;
    redisContext *redis = nullptr;

    // 限制时间内不能重复登录
    if (LOGIN_LOCK_TIME > 0) {
        cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            return setErrorResponse(response, "connect to cache failed");
        }

        switch (RedisUtils::LoginLock(cache, openid)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                return setErrorResponse(response, "lock login failed");
            }
            case REDIS_FAIL: {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                return setErrorResponse(response, "login too frequent");
            }
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
    }

    // 校验玩家身份
    if (!delegate_.loginCheck(request)) {
        return setErrorResponse(response, "login check failed");
    }

    UserId userId;
    std::string roleListStr;
    std::vector<RoleProfile> roles;
    // 直接在mysql中为玩家创建userId
    MYSQL* mysql = g_MysqlPoolManager.getCoreRecord()->take();
    if (!mysql) {
        return setErrorResponse(response, "connect to db failed");
    }

    if (!MysqlUtils::LoadOrCreateUser(mysql, openid, userId, roleListStr)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        return setErrorResponse(response, "load or create user from db failed");
    }

    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);

    if (roleListStr.length() > 0) {
        Document doc;
        doc.Parse(roleListStr.c_str());

        if (!doc.IsArray()) {
            return setErrorResponse(response, "parse role list failed");
        }

        for (SizeType i = 0; i < doc.Size(); i++) {
            const Value& v = doc[i];
            RoleProfile info;
            info.serverId = v["s"].GetInt();
            info.roleId = v["r"].GetInt();

            roles.push_back(std::move(info));
        }
    }

    // 刷新逻辑服列表信息
    refreshServerGroupData();

    // 生成登录临时token
    //std::string token = UUIDUtils::genUUID();
    std::string token = std::to_string(randInt());

    // 在cache中记录token
    cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        return setErrorResponse(response, "connect to cache failed");
    }

    switch (RedisUtils::SetLoginToken(cache, userId, token)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return setErrorResponse(response, "set token failed for db error");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false); // 归还连接
            return setErrorResponse(response, "set token failed");
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 将以上信息返回给客户端
    JsonWriter json;
    json.start();
    json.put("retCode", 0);
    json.put("userId", userId);
    json.put("token", token);
    json.putRawArray("servers", t_serverGroupData_);
    json.put("roles", roles);
    json.end();
    
    setResponse(response, std::string(json.content(), json.length()));
}

void LoginHandlerMgr::getProfile(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response) {
    if (!request->has("userId")) {
        return setErrorResponse(response, "missing parameter userId");
    }

    UserId userId = std::stoi((*request)["userId"]);

    if (!request->has("token")) {
        return setErrorResponse(response, "missing parameter token");
    }

    std::string token = (*request)["token"];

    if (!request->has("roleId")) {
        return setErrorResponse(response, "missing parameter roleId");
    }

    RoleId roleId = std::stoi((*request)["roleId"]);

    RoleProfile info;
    info.roleId = roleId;

    UserId uid = 0;
    ServerId sid = 0;
    std::list<std::pair<std::string, std::string>> pDatas;
    if (!delegate_.loadProfile(info.roleId, uid, info.serverId, pDatas)) {
        return setErrorResponse(response, "load role profile failed");
    }

    if (uid != userId) {
        return setErrorResponse(response, "not user's role");
    }

    info.pData = ProtoUtils::marshalDataFragments(pDatas);

    JsonWriter json;
    json.start();
    json.put("retCode", 0);
    json.put("profile", info);
    json.end();
    
    setResponse(response, std::string(json.content(), json.length()));
}

void LoginHandlerMgr::createRole(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response) {
    DEBUG_LOG("LoginHandlerMgr::createRole -- start ... \n");
    request->printAll();

    if (!request->has("userId")) {
        return setErrorResponse(response, "missing parameter userId");
    }

    UserId userId = std::stoi((*request)["userId"]);

    if (!request->has("token")) {
        return setErrorResponse(response, "missing parameter token");
    }

    std::string token = (*request)["token"];

    if (!request->has("serverId")) {
        return setErrorResponse(response, "missing parameter serverId");
    }

    ServerId serverId = std::stoi((*request)["serverId"]);

    // 刷新逻辑服列表信息
    refreshServerGroupData();

    // 判断服务器状态
    auto iter = t_serverId2groupIdMap_.find(serverId);
    if (iter == t_serverId2groupIdMap_.end()) {
        return setErrorResponse(response, "server not exist");
    }

    switch (t_groupStatusMap_[iter->second]) {
        case SERVER_STATUS_NORMAL:
            break;
        case SERVER_STATUS_FULL:
            return setErrorResponse(response, "server is full");
        case SERVER_STATUS_CLOSED:
            return setErrorResponse(response, "server is close");
        default:
            return setErrorResponse(response, "server status invalid");
    }

    // 校验token
    if (!checkToken(userId, token)) {
        return setErrorResponse(response, "check token failed");
    }

    // 创建角色数据
    std::list<std::pair<std::string, std::string>> roleDatas;
    if (!delegate_.createRole(request, roleDatas)) {
        return setErrorResponse(response, "create role failed");
    }

    // 设置创角锁（超时60秒），限制1个玩家1分钟内只能请求1次创角，防止攻击性创角
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        return setErrorResponse(response, "connect to cache failed");
    }

    switch (RedisUtils::CreateRoleLock(cache, userId)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return setErrorResponse(response, "lock create role failed");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            return setErrorResponse(response, "create role too frequent, please wait a minute and then retry");
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 从mysql中获取玩家角色id列表
    std::string roleListStr;
    std::vector<RoleServerInfo> roles;
    // 直接在mysql中为玩家创建userId
    MYSQL* mysql = g_MysqlPoolManager.getCoreRecord()->take();
    if (!mysql) {
        return setErrorResponse(response, "connect to db failed");
    }

    if (!MysqlUtils::LoadRoleIds(mysql, userId, roleListStr)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        return setErrorResponse(response, "load or create user from db failed");
    }

    if (roleListStr.length() > 0) {
        Document doc;
        doc.Parse(roleListStr.c_str());

        if (!doc.IsArray()) {
            g_MysqlPoolManager.getCoreRecord()->put(mysql, false);
            return setErrorResponse(response, "parse role list failed");
        }

        for (SizeType i = 0; i < doc.Size(); i++) {
            const Value& v = doc[i];
            RoleServerInfo info;
            info.serverId = v["s"].GetInt();
            info.roleId = v["r"].GetInt();

            roles.push_back(std::move(info));
        }
    }

    if (roles.size() >= g_LoginConfig.getPlayerRoleNumInAllServer()) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, false);
        return setErrorResponse(response, "reach the limit of the number of roles");
    }

    int roleNumInServer = 0;
    for (auto r : roles) {
        if (r.serverId == serverId) {
            roleNumInServer++;
        }
    }

    if (roleNumInServer >= g_LoginConfig.getPlayerRoleNumInOneServer()) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, false);
        return setErrorResponse(response, "reach the limit of the number of roles");
    }

    std::string rData = ProtoUtils::marshalDataFragments(roleDatas);
    
    RoleId roleId = 0;
    // 保存role，即上述第3步
    if (!MysqlUtils::CreateRole(mysql, roleId, userId, serverId, rData)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        return setErrorResponse(response, "save role to mysql failed");
    }

    assert(roleId != 0);
    // 更新user的role列表信息
    roles.push_back({serverId, roleId});
    JsonWriter jw;
    jw.put(roles);
    roleListStr.assign(jw.content(), jw.length());

    if (!MysqlUtils::UpdateRoleIds(mysql, userId, roleListStr, roles.size())) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        return setErrorResponse(response, "update role list failed");
    }

    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);

    // 缓存role
    cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        return setErrorResponse(response, "connect to cache failed");
    }

    switch (RedisUtils::SaveRole(cache, roleId, userId, serverId, roleDatas)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return setErrorResponse(response, "cache role failed for db error");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            return setErrorResponse(response, "cache role failed");
        }
    }

    // 缓存profile
    std::list<std::pair<std::string, std::string>> profileDatas;
    delegate_.makeProfile(roleDatas, profileDatas);
    switch (RedisUtils::SaveProfile(cache, roleId, userId, serverId, profileDatas)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("LoginHandlerMgr::createRole -- cache profile failed for db error\n");
            return setErrorResponse(response, "cache profile failed for db error");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("LoginHandlerMgr::createRole -- cache profile failed\n");
            return setErrorResponse(response, "cache profile failed");
        }
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);
    
    std::string pData = ProtoUtils::marshalDataFragments(profileDatas);
    RoleProfile role = {
        serverId: serverId,
        roleId: roleId,
        pData: std::move(pData)
    };

    JsonWriter json;
    json.start();
    json.put("retCode", 0);
    json.put("role", role);
    json.end();
    
    setResponse(response, std::string(json.content(), json.length()));
}

void LoginHandlerMgr::enterGame(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response) {
    if (!request->has("userId")) {
        return setErrorResponse(response, "missing parameter userId");
    }

    UserId userId = std::stoi((*request)["userId"]);

    if (!request->has("token")) {
        return setErrorResponse(response, "missing parameter token");
    }

    std::string token = (*request)["token"];

    if (!request->has("roleId")) {
        return setErrorResponse(response, "missing parameter roleId");
    }

    RoleId roleId = std::stoi((*request)["roleId"]);

    if (!request->has("serverId")) {
        return setErrorResponse(response, "missing parameter serverId");
    }

    ServerId serverId = std::stoi((*request)["serverId"]);

    // 校验token
    if (!checkToken(userId, token)) {
        return setErrorResponse(response, "check token failed");
    }

    MYSQL* mysql = g_MysqlPoolManager.getCoreRecord()->take();
    if (!mysql) {
        return setErrorResponse(response, "connect to db failed");
    }

    bool valid = false;
    if (!MysqlUtils::CheckRole(mysql, roleId, userId, serverId, valid)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        return setErrorResponse(response, "check role failed for db error");
    }

    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);

    if (!valid) {
        return setErrorResponse(response, "check role failed");
    }

    // 查询玩家Session（Session中记录了会话token，Gateway地址，角色id）
    // 若找到Session，则向Session中记录的Gateway发踢出玩家RPC请求
    // 若踢出返回失败，返回登录失败并退出（等待Redis中Session过期）
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        return setErrorResponse(response, "connect to cache failed");
    }

    ServerId orgGwId = 0;
    std::string orgTkn;
    RoleId orgRoleId = 0;
    switch (RedisUtils::GetSession(cache, userId, orgGwId, orgTkn, orgRoleId)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return setErrorResponse(response, "get session failed");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return setErrorResponse(response, "get session failed for return type invalid");
        }
    }

    if (orgGwId) {
        // 通知Gateway踢出玩家
        DEBUG_LOG("LoginHandlerMgr::enterGame -- user[%llu] role[%llu] duplicate login kick old one\n", userId, roleId);
        if (!g_GatewayClient.kick(orgGwId, userId, orgTkn)) {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            return setErrorResponse(response, "duplicate login kick old one failed");
        }
    }

    // 生成gToken（在游戏期间保持，用lua脚本保证操作原子性）
    //std::string gToken = UUIDUtils::genUUID();
    std::string gToken = std::to_string(randInt());

    ServerId gatewayId = 0;
    // 分配Gateway
    if (!g_ClientCenter.randomGatewayServer(gatewayId)) {
        g_RedisPoolManager.getCoreCache()->put(cache, false);
        return setErrorResponse(response, "cant find gateway");
    }

    // 若配置了前端服客户端通过前端服与gateway通信，否则客户端直接与gateway服连接
    Address targetAddr = g_LoginConfig.getFrontAddr();
    if (targetAddr.host.empty()) {
        targetAddr = g_ClientCenter.getGatewayAddress(gatewayId);
    }

    // TODO: 在passport信息中加入加密密钥
    // 客户端连接gateway后将passport转成session
    switch (RedisUtils::SetPassport(cache, userId, gatewayId, gToken, roleId)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return setErrorResponse(response, "set passport failed");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return setErrorResponse(response, "set passport failed for return type invalid");
        }
    }

    // 注意: 这里删不删LoginToken都一样（当并发请求多次enterGame时，token无法保证同时只有一个请求逻辑在进行）
    //if (RedisUtils::RemoveLoginToken(cache, userId) == REDIS_DB_ERROR) {
    //    g_RedisPoolManager.getCoreCache()->put(cache, true);
    //    return setErrorResponse(response, "delete token failed");
    //}

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    JsonWriter json;
    json.start();
    json.put("retCode", 0);
    json.put("gateId", gatewayId);
    json.put("host", targetAddr.host);
    json.put("port", targetAddr.port);
    json.put("gToken", gToken);
    json.end();
    
    setResponse(response, std::string(json.content(), json.length()));
}

/******************** http api end ********************/

void LoginHandlerMgr::updateServerGroupData() {
    redisContext *redis = g_RedisPoolManager.getCorePersist()->take();
    if (!redis) {
        ERROR_LOG("LoginHandlerMgr::_updateServerGroupData -- update server group data failed for cant connect db\n");
        return;
    }

    std::string serverGroupData;
    switch (RedisUtils::GetServerGroupsData(redis, serverGroupData)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCorePersist()->put(redis, true);
            ERROR_LOG("LoginHandlerMgr::_updateServerGroupData -- update server group data failed for db error");
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCorePersist()->put(redis, true);
            ERROR_LOG("LoginHandlerMgr::_updateServerGroupData -- update server group data failed for invalid data type\n");
            return;
        }
    }
    g_RedisPoolManager.getCorePersist()->put(redis, false);

    {
        LockGuard lock(serverGroupDataLock_);
        serverGroupData_ = std::move(serverGroupData);
        g_LoginHandlerMgr.updateServerGroupDataVersion();
    }
}

void LoginHandlerMgr::refreshServerGroupData() {
    if (t_serverGroupDataVersion_ != serverGroupDataVersion_) {
        std::string serverGroupData;
        uint32_t version;
        {
            LockGuard lock(serverGroupDataLock_);

            if (t_serverGroupDataVersion_ == serverGroupDataVersion_) {
                return;
            }

            serverGroupData = serverGroupData_;
            version = serverGroupDataVersion_;

            Document doc;
            if (doc.Parse(serverGroupData.c_str()).HasParseError()) {
                ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed\n");
                return;
            }

            if (!doc.IsArray()) {
                ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for invalid type\n");
                return;
            }

            std::map<GroupId, uint32_t> groupStatusMap;
            std::map<ServerId, GroupId> serverId2groupIdMap;
            for (SizeType i = 0; i < doc.Size(); i++) {
                const Value& group = doc[i];

                if (!group.HasMember("id")) {
                    ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for lack of group id\n");
                    return;
                }

                GroupId groupId = group["id"].GetInt();
                if (groupStatusMap.find(groupId) != groupStatusMap.end()) {
                    ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for group id %d duplicate\n", groupId);
                    return;
                }
                if (!group.HasMember("status")) {
                    ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for group %d status not define\n", groupId);
                    return;
                }

                uint32_t status = group["status"].GetInt();

                groupStatusMap.insert(std::make_pair(groupId, status));

                if (!group.HasMember("servers")) {
                    ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for group %d servers not define\n", groupId);
                    return;
                }

                const Value& servers = group["servers"];
                if (!servers.IsArray()) {
                    ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for group %d servers not array\n", groupId);
                    return;
                }

                for (SizeType i = 0; i < servers.Size(); i++) {
                    const Value& server = servers[i];

                    if (!server.IsObject()) {
                        ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for group %d servers[%d] not object\n", groupId, i);
                        return;
                    }

                    if (!server.HasMember("id")) {
                        ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server group data failed for group %d servers[%d] id not define\n", i);
                        return;
                    }

                    ServerId serverId = server["id"].GetInt();
                    if (serverId2groupIdMap.find(serverId) != serverId2groupIdMap.end()) {
                        ERROR_LOG("LoginHandlerMgr::refreshServerGroupData -- parse server id %d duplicate\n", serverId);
                        return;
                    }

                    serverId2groupIdMap.insert(std::make_pair(serverId, groupId));
                }
            }

            t_serverGroupData_ = std::move(serverGroupData);
            t_groupStatusMap_ = std::move(groupStatusMap);
            t_serverId2groupIdMap_ = std::move(serverId2groupIdMap);
            t_serverGroupDataVersion_ = version;
        }

    }
}

bool LoginHandlerMgr::checkToken(UserId userId, const std::string& token) {
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("LoginHandlerMgr::checkToken -- user[%llu] connect to cache failed\n", userId);
        return false;
    }

    std::string tkn;
    switch (RedisUtils::GetLoginToken(cache, userId, tkn)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            return false;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            return false;
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    if (token.compare(tkn) != 0) {
        ERROR_LOG("LoginHandlerMgr::checkToken -- user[%llu] token not match %s -- %s\n", userId, token.c_str(), tkn.c_str());
        return false;
    }

    return true;
}

void LoginHandlerMgr::setResponse(std::shared_ptr<ResponseMessage>& response, const std::string &content) {
    response->addHeader(HttpMessage::HEADER_CONTENT_TYPE, "application/json;charset=utf-8");
    response->setResult(HttpStatusCode::OK);
    response->setCapacity((uint32_t)content.size());
    response->appendContent(content);
}

void LoginHandlerMgr::setErrorResponse(std::shared_ptr<ResponseMessage>& response, const std::string &content) {
    response->addHeader(HttpMessage::HEADER_CONTENT_TYPE, "application/json;charset=utf-8");
    response->setResult(HttpStatusCode::OK);
    
    JsonWriter json;
    json.start();
    json.put("retCode", 1);
    json.put("error", content);
    json.end();
    
    response->setCapacity((uint32_t)json.length());
    response->appendContent(json.content());
}

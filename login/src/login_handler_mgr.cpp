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
#include "login_handler_mgr.h"
#include "json_utils.h"
#include "redis_utils.h"
#include "uuid_utils.h"
#include "login_config.h"
#include "rapidjson/document.h"
#include "share/const.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <unistd.h>
#include <random>

using namespace rapidjson;
using namespace wukong;

template<> void JsonWriter::put(const RoleInfo &value) {
    _writer.StartObject();
    _writer.Key("serverId");
    put(value.serverId);
    _writer.Key("roleId");
    put(value.roleId);
    _writer.Key("pData");
    put(value.pData);
    _writer.EndObject();
}

std::vector<GatewayClient::ServerInfo> LoginHandlerMgr::_gatewayInfos;
std::mutex LoginHandlerMgr::_gatewayInfosLock;
std::atomic<uint32_t> LoginHandlerMgr::_gatewayInfosVersion(0);
thread_local std::vector<ServerWeightInfo> LoginHandlerMgr::_t_gatewayInfos;
thread_local std::map<ServerId, Address> LoginHandlerMgr::_t_gatewayAddrMap;
thread_local uint32_t LoginHandlerMgr::_t_gatewayInfosVersion(0);
thread_local uint32_t LoginHandlerMgr::_t_gatewayTotalWeight(0);

std::string LoginHandlerMgr::_serverGroupData;
std::mutex LoginHandlerMgr::_serverGroupDataLock;
std::atomic<uint32_t> LoginHandlerMgr::_serverGroupDataVersion(0);
thread_local std::string LoginHandlerMgr::_t_serverGroupData;
thread_local uint32_t LoginHandlerMgr::_t_serverGroupDataVersion(0);
thread_local std::map<GroupId, uint32_t> LoginHandlerMgr::_t_groupStatusMap;
thread_local std::map<ServerId, GroupId> LoginHandlerMgr::_t_serverId2groupIdMap;

void *LoginHandlerMgr::updateRoutine(void *arg) {
    LoginHandlerMgr *self = (LoginHandlerMgr *)arg;
    self->_updateServerGroupData();

    while (true) {
        self->updateGatewayInfos();
        sleep(60);
    }
    
    return nullptr;
}

void LoginHandlerMgr::updateGatewayInfos() {
    std::vector<GatewayClient::ServerInfo> infos = g_GatewayClient.getServerInfos();
    {
        std::unique_lock<std::mutex> lock(_gatewayInfosLock);
        _gatewayInfos = std::move(infos);
        updateGatewayInfosVersion();
    }
    DEBUG_LOG("update gateway server info\n");
}

void *LoginHandlerMgr::initRoutine(void *arg) {
    LoginHandlerMgr *self = (LoginHandlerMgr *)arg;

    redisContext *cache = self->_cache->proxy.take();
    if (!cache) {
        ERROR_LOG("LoginHandlerMgr::initRoutine -- connect to cache failed\n");
        return nullptr;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_SESSION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("LoginHandlerMgr::initRoutine -- script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("LoginHandlerMgr::initRoutine -- script load failed\n");
        return nullptr;
    }

    self->_redisSetSessionSha1 = reply->str;
    self->_cache->proxy.put(cache, false);
    
    return nullptr;
}

void LoginHandlerMgr::init(HttpServer *server) {
    _cache = corpc::RedisConnectPool::create(g_LoginConfig.getCache().host.c_str(), g_LoginConfig.getCache().port, g_LoginConfig.getCache().dbIndex, g_LoginConfig.getCache().maxConnect);
    _db = corpc::RedisConnectPool::create(g_LoginConfig.getDB().host.c_str(), g_LoginConfig.getDB().port, g_LoginConfig.getDB().dbIndex, g_LoginConfig.getDB().maxConnect);
    
    RoutineEnvironment::startCoroutine(updateRoutine, this);

    // 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initRoutine, this);

    // 获取并订阅服务器组列表信息
    std::list<std::string> topics;
    topics.push_back("ServerGroups");
    PubsubService::StartPubsubService(g_LoginHandlerMgr.getCache(), topics);
    PubsubService::Subscribe("ServerGroups", false, std::bind(&LoginHandlerMgr::updateServerGroupData, this, std::placeholders::_1, std::placeholders::_2));
    
    // 注册filter
    // 验证request是否合法
    server->registerFilter([](std::shared_ptr<RequestMessage>& request, std::shared_ptr<ResponseMessage>& response) -> bool {
        // TODO: authentication
//        response->setResult(HttpStatusCode::Unauthorized);
//        response->addHeader("WWW-Authenticate", "Basic realm=\"Need Authentication\"");
        return true;
    });
    
    // 注册handler
    // req: no parameters
    // res: e.g. {"msg": "request sent."}
    server->registerHandler("/login", std::bind(&LoginHandlerMgr::login, this, std::placeholders::_1, std::placeholders::_2));
    // req: no parameters
    // res: e.g. {"ip": "127.0.0.1", "port": 8080}
    server->registerHandler("/createRole", std::bind(&LoginHandlerMgr::createRole, this, std::placeholders::_1, std::placeholders::_2));
    // req: roleid=73912798174
    // res: e.g. {"roleData": "xxx"}
    server->registerHandler("/enterGame", std::bind(&LoginHandlerMgr::enterGame, this, std::placeholders::_1, std::placeholders::_2));

    _loginCheck = [](std::shared_ptr<RequestMessage> &request) -> bool {
        return true;
    };

    _createRole = [](std::shared_ptr<RequestMessage> &request, std::string &pData) -> RoleId {
        return 0;
    };
}

/******************** http api start ********************/

void LoginHandlerMgr::login(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response) {
    if (!request->has("openid")) {
        return setErrorResponse(response, "missing parameter openid");
    }

    std::string openid = (*request)["openid"];

    // 校验玩家身份
    if (!_loginCheck(request)) {
        return setErrorResponse(response, "login check failed");
    }

    // 查询openid对应的userId（采用redis数据库）
    redisContext *redis = _db->proxy.take();
    if (!redis) {
        return setErrorResponse(response, "connect to db failed");
    }

    redisReply *reply = (redisReply *)redisCommand(redis, "GET O2U:%s", openid.c_str());
    if (!reply) {
        _db->proxy.put(redis, true);
        return setErrorResponse(response, "get user id from db failed");
    }

    std::vector<RoleInfo> roles;
    UserId userId;
    switch (reply->type) {
        case REDIS_REPLY_NIL: {
            // 若userId不存在，为玩家生成userId并记录openid与userId的关联关系（setnx）
            freeReplyObject(reply);

            userId = RedisUtils::CreateUserID(redis);
            if (userId == 0) {
                _db->proxy.put(redis, true);
                return setErrorResponse(response, "create user id failed");
            }

            reply = (redisReply *)redisCommand(redis, "SET O2U:%s %d NX", openid.c_str(), userId);
            if (!reply) {
                _db->proxy.put(redis, true);
                return setErrorResponse(response, "set user id for openid failed");
            } else if (strcmp(reply->str, "OK")) {
                freeReplyObject(reply);
                _db->proxy.put(redis, false); // 归还连接
                return setErrorResponse(response, "set userid for openid failed for already exist");
            }

            freeReplyObject(reply);
            break;
        }
        case REDIS_REPLY_STRING: {
            // 若userId存在，查询玩家所拥有的所有角色基本信息（从哪里查？在redis中记录userId关联的角色id列表，redis中还记录了每个角色的轮廓数据）
            userId = atoi(reply->str);

            freeReplyObject(reply);

            if (userId == 0) {
                _db->proxy.put(redis, false);
                return setErrorResponse(response, "invalid userid");
            }

            // 获取角色id列表
            reply = (redisReply *)redisCommand(redis, "SMEMBERS RoleIds:%d", userId);
            if (!reply) {
                _db->proxy.put(redis, true);
                return setErrorResponse(response, "get role id list failed");
            }

            if (reply->type != REDIS_REPLY_ARRAY) {
                freeReplyObject(reply);
                _db->proxy.put(redis, true);
                return setErrorResponse(response, "get role id list failed for return type invalid");
            }

            if (reply->elements > 0) {
                roles.reserve(reply->elements);
                for (int i = 0; i < reply->elements; i++) {
                    RoleInfo info;
                    info.serverId = 0;
                    info.roleId = atoi(reply->element[i]->str);
                    if (info.roleId == 0) {
                        freeReplyObject(reply);
                        _db->proxy.put(redis, false);
                        return setErrorResponse(response, "invalid roleid");
                    }

                    roles.push_back(std::move(info));
                }
            }
            freeReplyObject(reply);

            // 查询轮廓数据
            for (RoleInfo &info : roles){
                reply = (redisReply *)redisCommand(redis, "HGETALL RoleInfo:%d", info.roleId);
                if (!reply) {
                    _db->proxy.put(redis, true);
                    return setErrorResponse(response, "get role info failed");
                }

                if (reply->type != REDIS_REPLY_ARRAY || reply->elements % 2 != 0) {
                    freeReplyObject(reply);
                    _db->proxy.put(redis, true);
                    return setErrorResponse(response, "get role info failed for return type invalid");
                }

                if (reply->elements > 0) {
                    for (int i = 0; i < reply->elements; i += 2) {
                        if (strcmp(reply->element[i]->str, "serverId") == 0) {
                            info.serverId = atoi(reply->element[i+1]->str);
                        } else if (strcmp(reply->element[i]->str, "pData") == 0) {
                            info.pData = reply->element[i+1]->str;
                        }
                    }
                }

                freeReplyObject(reply);
            }

            break;
        }
        default: {
            freeReplyObject(reply);
            _db->proxy.put(redis, true);
            return setErrorResponse(response, "get user id from db failed for invalid data type");
        }
    }

    _db->proxy.put(redis, false);

    // 刷新逻辑服列表信息
    refreshServerGroupData();

    // 生成登录临时token
    std::string token = UUIDUtils::genUUID();

    // 在cache中记录token
    redisContext *cache = _cache->proxy.take();
    if (!cache) {
        return setErrorResponse(response, "connect to cache failed");
    }

    // 以Token:[userId]为key，value为token，记录到redis数据库（有效期1小时？创角时延长有效期，玩家进入游戏时清除token）
    reply = (redisReply *)redisCommand(cache, "SET Token:%d %s EX 3600", userId, token.c_str());
    if (!reply) {
        _cache->proxy.put(cache, true);
        return setErrorResponse(response, "set token failed for db error");
    } else if (strcmp(reply->str, "OK")) {
        freeReplyObject(reply);
        _cache->proxy.put(cache, false); // 归还连接
        return setErrorResponse(response, "set token failed");
    }

    freeReplyObject(reply);

    _cache->proxy.put(cache, false);

    // 将以上信息返回给客户端
    JsonWriter json;
    json.start();
    json.put("retCode", 0);
    json.put("userId", userId);
    json.put("token", token);
    json.putRawArray("servers", _t_serverGroupData);
    json.put("roles", roles);
    json.end();
    
    setResponse(response, std::string(json.content(), json.length()));
}

void LoginHandlerMgr::createRole(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response) {
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

    // 判断服务器状态
    auto iter = _t_serverId2groupIdMap.find(serverId);
    if (iter == _t_serverId2groupIdMap.end()) {
        return setErrorResponse(response, "server not exist");
    }

    switch (_t_groupStatusMap[iter->second]) {
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

    // 创角
    std::string pData;
    RoleId roleId = _createRole(request, pData);
    if (roleId == 0) {
        return setErrorResponse(response, "create role failed");
    }

    RoleInfo role = {
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

    // 校验token
    if (!checkToken(userId, token)) {
        return setErrorResponse(response, "check token failed");
    }

    redisContext *redis = _db->proxy.take();
    if (!redis) {
        return setErrorResponse(response, "connect to db failed");
    }

    // 判断角色是否合法
    ServerId serverId;
    redisReply *reply = (redisReply *)redisCommand(redis, "HGETALL RoleInfo:%d", roleId);
    if (!reply) {
        _db->proxy.put(redis, true);
        return setErrorResponse(response, "get role info failed");
    }

    if (reply->type != REDIS_REPLY_ARRAY || reply->elements % 2 != 0) {
        freeReplyObject(reply);
        _db->proxy.put(redis, true);
        return setErrorResponse(response, "get role info failed for return type invalid");
    }

    if (reply->elements > 0) {
        bool role_valid = false;
        for (int i = 0; i < reply->elements; i += 2) {
            if (strcmp(reply->element[i]->str, "serverId") == 0) {
                serverId = atoi(reply->element[i+1]->str);
            } else if (strcmp(reply->element[i]->str, "userId") == 0) {
                role_valid = atoi(reply->element[i+1]->str) == userId;
            }
        }

        freeReplyObject(reply);

        if (!role_valid) {
            _db->proxy.put(redis, false);
            return setErrorResponse(response, "role invalid");
        }
    } else {
        freeReplyObject(reply);
        _db->proxy.put(redis, false);

        return setErrorResponse(response, "role not exist");
    }

    _db->proxy.put(redis, false);

    // 查询玩家Session（Session中记录了会话token，Gateway地址，角色id）
    // 若找到Session，则向Session中记录的Gateway发踢出玩家RPC请求
    // 若踢出返回失败，返回登录失败并退出（等待Redis中Session过期）
    redisContext *cache = _cache->proxy.take();
    if (!cache) {
        return setErrorResponse(response, "connect to cache failed");
    }

    reply = (redisReply *)redisCommand(cache, "HGETALL session:%d", userId);
    if (!reply) {
        _cache->proxy.put(cache, true);
        return setErrorResponse(response, "get session failed");
    }

    if (reply->type != REDIS_REPLY_ARRAY || reply->elements % 2 != 0) {
        freeReplyObject(reply);
        _cache->proxy.put(cache, true);
        return setErrorResponse(response, "get session failed for return type invalid");
    }

    if (reply->elements > 0) {
        // 通知Gateway踢出玩家
        ServerId gid = 0;
        RoleId rid = 0;
        for (int i = 0; i < reply->elements; i += 2) {
            if (strcmp(reply->element[i]->str, "gateId") == 0) {
                gid = atoi(reply->element[i+1]->str);
            } else if (strcmp(reply->element[i]->str, "roleId") == 0) {
                rid = atoi(reply->element[i+1]->str);
            }
        }

        freeReplyObject(reply);

        if (!g_GatewayClient.kick(gid, rid)) {
            _cache->proxy.put(cache, false);
            return setErrorResponse(response, "duplicate login kick old one failed");
        }
    }

    // 生成gToken（在游戏期间保持，用lua脚本保证操作原子性）
    std::string gToken = UUIDUtils::genUUID();

    ServerId gatewayId = 0;
    // 分配Gateway
    if (!randomGatewayServer(gatewayId)) {
        _cache->proxy.put(cache, false);
        return setErrorResponse(response, "cant find gateway");
    }

    Address gatewayAddr = _t_gatewayAddrMap[gatewayId];

    if (_redisSetSessionSha1.empty()) {
        reply = (redisReply *)redisCommand(cache, "eval %s 1 session:%d %s %d %d", SET_SESSION_CMD, userId, gToken.c_str(), gatewayId, roleId);
    } else {
        reply = (redisReply *)redisCommand(cache, "evalsha %s 1 session:%d %s %d %d", _redisSetSessionSha1.c_str(), userId, gToken.c_str(), gatewayId, roleId);
    }
    
    if (!reply) {
        _cache->proxy.put(cache, true);
        return setErrorResponse(response, "set session failed");
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        _cache->proxy.put(cache, true);
        return setErrorResponse(response, "set session failed for return type invalid");
    }

    if (reply->integer == 0) {
        // 设置失败
        freeReplyObject(reply);
        _cache->proxy.put(cache, false);
        return setErrorResponse(response, "set session failed for already set");
    }

    // 删除登录临时token
    reply = (redisReply *)redisCommand(cache,"DEL Token:%d", userId);
    if (!reply) {
        _cache->proxy.put(cache, true);
        return setErrorResponse(response, "delete token failed");
    }

    freeReplyObject(reply);
    _cache->proxy.put(cache, false);

    JsonWriter json;
    json.start();
    json.put("host", gatewayAddr.host);
    json.put("port", gatewayAddr.port);
    json.put("gToken", gToken);
    json.end();
    
    setResponse(response, std::string(json.content(), json.length()));
}

/******************** http api end ********************/

bool LoginHandlerMgr::randomGatewayServer(ServerId &serverId) {
    refreshGatewayInfos();
    size_t serverNum = _t_gatewayInfos.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = _t_gatewayTotalWeight;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += _t_gatewayInfos[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = _t_gatewayInfos[i].id;

    // 调整权重
    _t_gatewayTotalWeight += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        _t_gatewayInfos[j].weight++;
    }
    _t_gatewayInfos[i].weight--;
    return true;
}

void LoginHandlerMgr::refreshGatewayInfos() {
    if (_t_gatewayInfosVersion != _gatewayInfosVersion) {
        _t_gatewayInfos.clear();
        _t_gatewayAddrMap.clear();

        std::vector<GatewayClient::ServerInfo> gatewayInfos;

        {
            std::unique_lock<std::mutex> lock(_gatewayInfosLock);
            gatewayInfos = _gatewayInfos;
            _t_gatewayInfosVersion = _gatewayInfosVersion;
        }

        _t_gatewayInfos.reserve(gatewayInfos.size());
        uint32_t totalWeight = 0;
        for (auto &info : gatewayInfos) {
            totalWeight += info.weight;

            _t_gatewayInfos.push_back({info.id, info.weight});

            Address gatewayAddr = {info.outerAddr, info.outerPort};
            _t_gatewayAddrMap.insert(std::make_pair(info.id, std::move(gatewayAddr)));
        }
        
        _t_gatewayTotalWeight = totalWeight;
    }
}

void LoginHandlerMgr::updateServerGroupData(const std::string& topic, const std::string& msg) {
    _updateServerGroupData();
}

void LoginHandlerMgr::_updateServerGroupData() {
    redisContext *redis = _db->proxy.take();
    if (!redis) {
        ERROR_LOG("update server group data failed for cant connect db\n");
        return;
    }

    redisReply *reply = (redisReply *)redisCommand(redis, "GET ServerGroups");
    if (!reply) {
        _db->proxy.put(redis, true);
        ERROR_LOG("update server group data failed for db error");
        return;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        _db->proxy.put(redis, true);
        ERROR_LOG("update server group data failed for invalid data type\n");
        return;
    }

    std::string serverGroupData(reply->str, reply->len);
    {
        std::unique_lock<std::mutex> lock(_serverGroupDataLock);
        _serverGroupData = std::move(serverGroupData);
        g_LoginHandlerMgr.updateServerGroupDataVersion();
    }
}

void LoginHandlerMgr::refreshServerGroupData() {
    if (_t_serverGroupDataVersion != _serverGroupDataVersion) {
        std::string serverGroupData;
        uint32_t version;
        {
            std::unique_lock<std::mutex> lock(_serverGroupDataLock);
            serverGroupData = _serverGroupData;
            version = _serverGroupDataVersion;
        }

        Document doc;
        if (doc.Parse(serverGroupData.c_str()).HasParseError()) {
            ERROR_LOG("parse server group data failed\n");
            return;
        }

        if (!doc.IsArray()) {
            ERROR_LOG("parse server group data failed for invalid type\n");
            return;
        }

        std::map<GroupId, uint32_t> groupStatusMap;
        std::map<ServerId, GroupId> serverId2groupIdMap;
        for (SizeType i = 0; i < doc.Size(); i++) {
            const Value& group = doc[i];

            if (!doc.HasMember("id")) {
                ERROR_LOG("parse server group data failed for lack of group id\n");
                return;
            }

            GroupId groupId = doc["id"].GetInt();
            if (groupStatusMap.find(groupId) != groupStatusMap.end()) {
                ERROR_LOG("parse server group data failed for group id %d duplicate\n", groupId);
                return;
            }
            if (!doc.HasMember("status")) {
                ERROR_LOG("parse server group data failed for group %d status not define\n", groupId);
                return;
            }

            uint32_t status = doc["status"].GetInt();

            groupStatusMap.insert(std::make_pair(groupId, status));

            if (!doc.HasMember("servers")) {
                ERROR_LOG("parse server group data failed for group %d servers not define\n", groupId);
                return;
            }

            const Value& servers = doc["servers"];
            if (!servers.IsArray()) {
                ERROR_LOG("parse server group data failed for group %d servers not array\n", groupId);
                return;
            }

            for (SizeType i = 0; i < servers.Size(); i++) {
                const Value& server = servers[i];

                if (!server.IsObject()) {
                    ERROR_LOG("parse server group data failed for group %d servers[%d] not object\n", groupId, i);
                    return;
                }

                if (!server.HasMember("id")) {
                    ERROR_LOG("parse server group data failed for group %d servers[%d] id not define\n", i);
                    return;
                }

                ServerId serverId = server["id"].GetInt();
                if (serverId2groupIdMap.find(serverId) != serverId2groupIdMap.end()) {
                    ERROR_LOG("parse server id %d duplicate\n", serverId);
                    return;
                }

                serverId2groupIdMap.insert(std::make_pair(serverId, groupId));
            }
        }

        _t_serverGroupData = std::move(serverGroupData);
        _t_groupStatusMap = std::move(groupStatusMap);
        _t_serverId2groupIdMap = std::move(serverId2groupIdMap);
        _t_serverGroupDataVersion = version;
    }
}

bool LoginHandlerMgr::checkToken(UserId userId, const std::string& token) {
    redisContext *cache = _cache->proxy.take();
    if (!cache) {
        ERROR_LOG("connect to cache failed\n");
        return false;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "GET Token:%d", userId);
    if (!reply) {
        _db->proxy.put(cache, true);
        return false;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        _db->proxy.put(cache, false);
        return false;
    }

    if (token.compare(reply->str) != 0) {
        freeReplyObject(reply);
        _db->proxy.put(cache, false);
        return false;
    }

    freeReplyObject(reply);
    _db->proxy.put(cache, false);
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

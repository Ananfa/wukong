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
    template<> void JsonWriter::put(const RoleProfile &value) {
        _writer.StartObject();
        _writer.Key("serverId");
        put(value.serverId);
        _writer.Key("roleId");
        put(value.roleId);
        _writer.Key("pData");
        put(value.pData);
        _writer.EndObject();
    }
}

std::string LoginHandlerMgr::_serverGroupData;
Mutex LoginHandlerMgr::_serverGroupDataLock;
std::atomic<uint32_t> LoginHandlerMgr::_serverGroupDataVersion(0);
thread_local std::string LoginHandlerMgr::_t_serverGroupData("[]");
thread_local uint32_t LoginHandlerMgr::_t_serverGroupDataVersion(0);
thread_local std::map<GroupId, uint32_t> LoginHandlerMgr::_t_groupStatusMap;
thread_local std::map<ServerId, GroupId> LoginHandlerMgr::_t_serverId2groupIdMap;

void *LoginHandlerMgr::saveUserRoutine(void *arg) {
    LoginHandlerMgr *self = (LoginHandlerMgr *)arg;

    AccountUserIdInfoQueue& queue = self->_queue;

    int readFd = queue.getReadFd();
    co_register_fd(readFd);
    co_set_timeout(readFd, -1, 1000);
    int ret;
    std::vector<char> buf(1024);
    while (true) {
        // 等待处理信号
        ret = (int)read(readFd, &buf[0], 1024);
        assert(ret != 0);
        if (ret < 0) {
            if (errno == EAGAIN) {
                continue;
            } else {
                // 管道出错
                ERROR_LOG("LoginHandlerMgr::saveUserRoutine -- read from pipe fd %d ret %d errno %d (%s)\n",
                       readFd, ret, errno, strerror(errno));
                
                // TODO: 如何处理？退出协程？
                // sleep 10 milisecond
                msleep(10);
            }
        }

        MYSQL* mysql = nullptr;
        AccountUserIdInfo *msg = queue.pop();
        while (msg) {
            if (!mysql) {
                mysql = g_MysqlPoolManager.getCoreRecord()->take();
            }

            bool saved = false;
            if (mysql) {
                saved = MysqlUtils::SaveUser(mysql, msg->account, msg->userId);
            }

            if (!saved) {
                if (mysql) {
                    g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
                    mysql = nullptr;
                }

                ERROR_LOG("LoginHandlerMgr::saveUserRoutine -- save account:[%s], userid:[%d] to mysql failed\n", msg->account.c_str(), msg->userId);
            }

            delete msg;
            msg = queue.pop();
        }

        if (mysql) {
            g_MysqlPoolManager.getCoreRecord()->put(mysql, false);
        }
    }
}

void LoginHandlerMgr::init(HttpServer *server) {
    RoutineEnvironment::startCoroutine(saveUserRoutine, this);

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
    // req: no parameters
    // res: e.g. {"msg": "request sent."}
    server->registerHandler(POST, "/login", std::bind(&LoginHandlerMgr::login, this, std::placeholders::_1, std::placeholders::_2));
    // req: no parameters
    // res: e.g. {"ip": "127.0.0.1", "port": 8080}
    server->registerHandler(POST, "/createRole", std::bind(&LoginHandlerMgr::createRole, this, std::placeholders::_1, std::placeholders::_2));
    // req: roleid=73912798174
    // res: e.g. {"roleData": "xxx"}
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
    if (!_delegate.loginCheck(request)) {
        return setErrorResponse(response, "login check failed");
    }

    // 查询openid对应的userId（采用redis数据库）
    redis = g_RedisPoolManager.getCorePersist()->take();
    if (!redis) {
        return setErrorResponse(response, "connect to db failed");
    }

    UserId userId;
    std::vector<RoleProfile> roles;
    switch (RedisUtils::GetUserID(redis, openid, userId)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCorePersist()->put(redis, true);
            return setErrorResponse(response, "get user id from db failed");
        }
        case REDIS_FAIL: {
            // 若userId不存在，为玩家生成userId并记录openid与userId的关联关系（setnx）

            // 问题: 是否需要将openid与userid的关系存到mysql中？若存mysql，是否需要通过分库分表来提高负载能力？云数据库（分布式数据库）应该有这个能力
            // 答：redis数据库主从备份设计以及数据库日志能保证数据安全性，只需要进行tlog日志记录就可以（tlog日志会进行入库处理），另外，也可以存到mysql中防止redis库损坏

            // 这里是否增加一步从mysql数据库查userid？查到就写入redis？由于角色列表也是存在redis中，只查userid不够，因此不用从mysql查了

            userId = RedisUtils::CreateUserID(redis);
            if (userId == 0) {
                g_RedisPoolManager.getCorePersist()->put(redis, true);
                return setErrorResponse(response, "create user id failed");
            }

            switch (RedisUtils::SetUserID(redis, openid, userId)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCorePersist()->put(redis, true);
                    return setErrorResponse(response, "set user id for openid failed");
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCorePersist()->put(redis, false); // 归还连接
                    return setErrorResponse(response, "set userid for openid failed for already exist");
                }
            }

            g_RedisPoolManager.getCorePersist()->put(redis, false);

            // TODO: tlog记录openid与userid的关系

            // 是同步记录还是异步记录好？异步记录会有丢失风险
            // 这里通过协程异步将信息存到mysql中，有可能存储失败，若存储失败查记录日志
            AccountUserIdInfo *a2uInfo = new AccountUserIdInfo();
            a2uInfo->account = openid;
            a2uInfo->userId = userId;
            _queue.push(a2uInfo);

            break;
        }
        case REDIS_SUCCESS: {
            // 若userId存在，查询玩家所拥有的所有角色基本信息（从哪里查？在redis中记录userId关联的角色id列表，redis中还记录了每个角色的轮廓数据）
            if (userId == 0) {
                g_RedisPoolManager.getCorePersist()->put(redis, false);
                return setErrorResponse(response, "invalid userid");
            }

            std::vector<RoleId> roleIds;
            switch (RedisUtils::GetUserRoleIdList(redis, userId, roleIds)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCorePersist()->put(redis, true);
                    return setErrorResponse(response, "get role id list failed");
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCorePersist()->put(redis, true);
                    return setErrorResponse(response, "get role id list failed for return type invalid");
                }
            }
            g_RedisPoolManager.getCorePersist()->put(redis, false);

            roles.reserve(roleIds.size());
            for (RoleId roleId : roleIds) {
                if (roleId == 0) {
                    return setErrorResponse(response, "invalid roleid");
                }

                RoleProfile info;
                info.serverId = 0;
                info.roleId = roleId;
                roles.push_back(std::move(info));
            }

            // 查询轮廓数据
            for (RoleProfile &info : roles){
                UserId uid = 0;
                std::list<std::pair<std::string, std::string>> pDatas;
                if (!_delegate.loadProfile(info.roleId, uid, info.serverId, pDatas)) {
                    ERROR_LOG("LoginHandlerMgr::login -- load role %d profile failed\n", info.roleId);
                    continue;
                }
                assert(uid == userId);
                info.pData = ProtoUtils::marshalDataFragments(pDatas);
            }

            break;
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
    json.putRawArray("servers", _t_serverGroupData);
    json.put("roles", roles);
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

    // 创建角色数据
    std::list<std::pair<std::string, std::string>> roleDatas;
    if (!_delegate.createRole(request, roleDatas)) {
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

    // 判断同服角色数量限制
    redisContext *redis = g_RedisPoolManager.getCorePersist()->take();
    if (!redis) {
        return setErrorResponse(response, "connect to redis failed");
    }

    uint32_t count = 0;
    if (RedisUtils::GetRoleCount(redis, userId, serverId, count) == REDIS_DB_ERROR) {
        g_RedisPoolManager.getCorePersist()->put(redis, true);
        return setErrorResponse(response, "get role count failed");
    }

    if (count >= g_LoginConfig.getRoleNumForPlayer()) {
        g_RedisPoolManager.getCorePersist()->put(redis, false);
        return setErrorResponse(response, "reach the limit of the number of roles");
    }

    // 生成roleId
    RoleId roleId = RedisUtils::CreateRoleID(redis);
    if (roleId == 0) {
        g_RedisPoolManager.getCorePersist()->put(redis, true);
        return setErrorResponse(response, "create role id failed");
    }
    
    // 完成以下事情，其中1和2可以通过redis lua脚本一起完成。有可能出现1和2操作完成但3或4或5没有操作，此时只能玩家找客服后通过工具修复玩家的RoleIds数据了。
    //       1. 将roleId加入RoleIds:<serverid>:{<userid>}中，保证玩家角色数量不超过区服角色数量限制
    //       2. 将roleId加入RoleIds:{<userid>}
    //       3. 将角色数据存盘mysql（若存盘失败，将上面1、2步骤回退）
    //       4. 将角色数据记录到Role:{<roleid>}中，并加入相应的存盘列表
    //       5. 记录角色轮廓数据Profile:{<roleid>}（注意：记得加serverId）
    
    // 绑定role，即上述第1和第2步
    switch (RedisUtils::BindRole(redis, roleId, userId, serverId, g_LoginConfig.getRoleNumForPlayer())) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCorePersist()->put(redis, true);
            ERROR_LOG("LoginHandlerMgr::createRole -- bind roleId:[%d] userId:[%d] serverId:[%d] failed for db error\n", roleId, userId, serverId);
            return setErrorResponse(response, "bind role failed for db error");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCorePersist()->put(redis, false);
            ERROR_LOG("LoginHandlerMgr::createRole -- bind roleId:[%d] userId:[%d] serverId:[%d] failed\n", roleId, userId, serverId);
            return setErrorResponse(response, "bind role failed");
        }
    }

    g_RedisPoolManager.getCorePersist()->put(redis, false);

    std::string rData = ProtoUtils::marshalDataFragments(roleDatas);
    
    MYSQL *mysql = g_MysqlPoolManager.getCoreRecord()->take();
    if (!mysql) {
        return setErrorResponse(response, "connect to mysql failed");
    }
    
    // 保存role，即上述第3步
    if (!MysqlUtils::CreateRole(mysql, roleId, userId, serverId, rData)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        return setErrorResponse(response, "save role to mysql failed");
    }

    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);

    // 缓存role，即上述第4步
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

    // 缓存profile，即上述第5步
    std::list<std::pair<std::string, std::string>> profileDatas;
    _delegate.makeProfile(roleDatas, profileDatas);
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

    redisContext *redis = g_RedisPoolManager.getCorePersist()->take();
    if (!redis) {
        return setErrorResponse(response, "connect to db failed");
    }

    bool valid = false;
    switch (RedisUtils::CheckRole(redis, userId, serverId, roleId, valid)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCorePersist()->put(redis, true);
            return setErrorResponse(response, "check role failed");
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCorePersist()->put(redis, true);
            return setErrorResponse(response, "check role failed for return type invalid");
        }
    }
    g_RedisPoolManager.getCorePersist()->put(redis, false);

    if (!valid) {
        return setErrorResponse(response, "role check failed");
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

    Address gatewayAddr = g_ClientCenter.getGatewayAddress(gatewayId);

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

    if (RedisUtils::RemoveLoginToken(cache, userId) == REDIS_DB_ERROR) {
        g_RedisPoolManager.getCoreCache()->put(cache, true);
        return setErrorResponse(response, "delete token failed");
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    JsonWriter json;
    json.start();
    json.put("retCode", 0);
    json.put("host", gatewayAddr.host);
    json.put("port", gatewayAddr.port);
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
        LockGuard lock(_serverGroupDataLock);
        _serverGroupData = std::move(serverGroupData);
        g_LoginHandlerMgr.updateServerGroupDataVersion();
    }
}

void LoginHandlerMgr::refreshServerGroupData() {
    if (_t_serverGroupDataVersion != _serverGroupDataVersion) {
        std::string serverGroupData;
        uint32_t version;
        {
            LockGuard lock(_serverGroupDataLock);

            if (_t_serverGroupDataVersion == _serverGroupDataVersion) {
                return;
            }

            serverGroupData = _serverGroupData;
            version = _serverGroupDataVersion;

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

            _t_serverGroupData = std::move(serverGroupData);
            _t_groupStatusMap = std::move(groupStatusMap);
            _t_serverId2groupIdMap = std::move(serverId2groupIdMap);
            _t_serverGroupDataVersion = version;
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

/*
 * Created by Xianke Liu on 2020/12/22.
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

#include "gateway_handler.h"
#include "redis_pool.h"
//#include "client_center.h"
#include "agent_manager.h"
#include "lobby_agent.h"
#include "share/const.h"
#include "string_utils.h"
#include "redis_utils.h"
#include "gateway_config.h"
#include "gateway_object_manager.h"

#include "game.pb.h"

using namespace wukong;

void GatewayHandler::registerMessages(corpc::MessageTerminal *terminal) {
    terminal->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false, GatewayHandler::connectHandle);
    terminal->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, true, GatewayHandler::closeHandle);
    terminal->registerMessage(CORPC_MSG_TYPE_BANNED, nullptr, true, GatewayHandler::banHandle);
    terminal->registerMessage(C2S_MESSAGE_ID_AUTH, new pb::AuthRequest, true, GatewayHandler::authHandle);

    terminal->setOtherMessageHandle(GatewayHandler::bypassHandle);
}

void GatewayHandler::connectHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    // 登记未认证连接
    DEBUG_LOG("GatewayHandler::connectHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());

    if (g_GatewayObjectManager.isShutdown()) {
        conn->close();
    } else {
        if (conn->isOpen()) {
            g_GatewayObjectManager.addUnauthConn(conn);
        } else {
            DEBUG_LOG("GatewayHandler::connectHandle -- conn %d[%d] is not open\n", conn.get(), conn->getfd());
        }
    }
}

void GatewayHandler::closeHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    DEBUG_LOG("GatewayHandler::closeHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());
    assert(!conn->isOpen());
    if (g_GatewayObjectManager.isUnauth(conn)) {
        g_GatewayObjectManager.removeUnauthConn(conn);
    } else if (!g_GatewayObjectManager.tryMoveToDisconnectedLink(conn)) {
        // 注意：出现这种情况的原因是在认证过程中断线，或者玩家被踢出时
        DEBUG_LOG("GatewayHandler::closeHandle -- connection close but cant move to disconnected list\n");
    }
}

void GatewayHandler::banHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    DEBUG_LOG("GatewayHandler::banHandle -- msgType:%d\n", type);
    // 向客户端发屏蔽消息
    std::shared_ptr<pb::BanResponse> response(new pb::BanResponse);
    response->set_msgid(type);

    conn->send(S2C_MESSAGE_ID_BAN, false, false, true, tag, response);
}

void GatewayHandler::authHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    DEBUG_LOG("GatewayHandler::authHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());
    if (g_GatewayObjectManager.isShutdown()) {
        ERROR_LOG("GatewayHandler::authHandle -- server shutdown\n");
        conn->close();
        return;
    }

    if (!conn->isOpen()) {
        ERROR_LOG("GatewayHandler::authHandle -- connection closed when begin auth\n");
        return;
    }

    // 确认连接是未认证连接（不允许重复发认证消息）（调试发现：也可能是auth消息处理在connect消息处理之前发生导致--有概率发生）
    if (!g_GatewayObjectManager.isUnauth(conn)) {
        ERROR_LOG("GatewayHandler::authHandle -- not an unauth connection\n");
        conn->close();
        return;
    }

    g_GatewayObjectManager.removeUnauthConn(conn);

    pb::AuthRequest *request = static_cast<pb::AuthRequest*>(msg.get());
    UserId userId = request->userid();
    ServerId gateId = request->gateid();
    const std::string &gToken = request->token();

    DEBUG_LOG("GatewayHandler::authHandle -- userId:%d\n", userId);

    if (gateId != g_GatewayConfig.getId()) {
        ERROR_LOG("GatewayHandler::authHandle -- gateid not match\n");
        conn->close();
        return;
    }

    // 若本地网关对象存在，直接跟本地网关对象的gToken比较，不需访问redis
    int ret = g_GatewayObjectManager.tryChangeGatewayObjectConn(userId, gToken, conn);
    if (ret == -1) {
        ERROR_LOG("GatewayHandler::authHandle -- reconnect token not match\n");
        conn->close();
        return;
    }

    // 设置通讯密钥
    // TODO: 密钥应该通过RSA加密，需要解码密钥才能使用，密钥应该在login服entergame时产生并传给客户端
    std::shared_ptr<Crypter> crypter(new SimpleXORCrypter(request->cipher()));
    conn->setCrypter(crypter);

    if (ret == 1) {
        // 此处是断线重连
        // 在新连接下补发未收到的消息
        if (conn->getMsgBuffer()->broken()) {
            conn->setLastSendSerial(request->recvserial());
            conn->getMsgBuffer()->reset();

            // 通知客户端补发消息失败，客户端应重新向服务器获取并同步数据
            conn->send(S2C_MESSAGE_ID_RESENDFAIL, true, false, false, 0, nullptr);
        } else {
            conn->scrapMessages(request->recvserial());
            conn->resend();

            // 这里加一个重连完成消息，客户端收到此消息后才可以向服务器发消息
            conn->send(S2C_MESSAGE_ID_RECONNECTED, true, false, false, 0, nullptr);
        }

        return;
    }

    // 获取session比对gateId和gtoken
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("GatewayHandler::authHandle -- connect to cache failed\n");
        conn->close();
        return;
    }

    // 用lua脚本校验passport信息——进行服务器号和token比较，返回roleId
    RoleId roleId = 0;
    switch (RedisUtils::CheckPassport(cache, userId, gateId, gToken, roleId)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GatewayHandler::authHandle -- user %d check passport failed for db error\n", userId);
            conn->close();
            return;
        }
        case REDIS_REPLY_INVALID: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- user %d check passport failed for redis reply invalid\n", userId);
            conn->close();
            return;
        }
    }

    // 为了防止重复连接，由于协程存在穿插执行的情况，程序执行到这里时可能已经产生了网关对象，在创建网关对象前应校验一下
    if (g_GatewayObjectManager.hasGatewayObject(userId)) {
        g_RedisPoolManager.getCoreCache()->put(cache, false);
        ERROR_LOG("GatewayHandler::authHandle -- user %d gateway object already exist\n", userId);
        conn->close();
        return;
    }

    // 若连接已断线，则不再继续创建网关对象
    if (!conn->isOpen()) {
        g_RedisPoolManager.getCoreCache()->put(cache, false);
        ERROR_LOG("GatewayHandler::authHandle -- user %d disconnected before create gateway object\n", userId);
        return;
    }

    // 设置Session
    switch (RedisUtils::SetSession(cache, userId, gateId, gToken, roleId)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GatewayHandler::authHandle -- user %d set session failed for db error\n", userId);
            conn->close();
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- user %d set session failed for already set\n", userId);
            conn->close();
            return;
        }
    }

    assert(!g_GatewayObjectManager.hasGatewayObject(userId));

    // 创建玩家网关对象
    std::shared_ptr<GatewayObject> obj = std::make_shared<GatewayObject>(userId, roleId, gToken, conn);

    // 队伍成员同时登录时会导致进入游戏失败，且passport失效（因此需要重试）
    int leftTryTimes = 3;
    bool success = false;
    while (!success) {
        // 查询玩家角色游戏对象所在（cache中key为Location:<roleId>的hash，由游戏对象负责维持心跳）
        //    若查到，设置玩家网关对象中的gameServerStub
        //    若没有查到，分配一个大厅服，并通知大厅服加载玩家游戏对象
        //       若返回失败，则登录失败
        std::string lToken;
        ServerId lobbyId;
        switch (RedisUtils::GetLobbyAddress(cache, roleId, lobbyId, lToken)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("GatewayHandler::authHandle -- get location failed for db error\n");
                conn->close();
                return;
            }
            case REDIS_REPLY_INVALID: {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                ERROR_LOG("GatewayHandler::authHandle -- get location failed for reply invalid\n");
                conn->close();
                return;
            }
            case REDIS_SUCCESS: {
                obj->setRelateServer(SERVER_TYPE_LOBBY, lobbyId);
                obj->setLToken(lToken);
                success = true;
                continue;
            }
        }

        // 向Lobby服发loadRole RPC前先释放数据库连接
        g_RedisPoolManager.getCoreCache()->put(cache, false);

        if (leftTryTimes > 0) {
            leftTryTimes--;
            // 分配一个Lobby服，并通知Lobby服加载玩家游戏对象
            ServerId sid = 0;

            LobbyAgent *lobbyAgent = (LobbyAgent*)g_AgentManager.getAgent(SERVER_TYPE_LOBBY);
            if (!lobbyAgent->randomServer(sid)) {
                ERROR_LOG("GatewayHandler::authHandle -- random lobby server failed\n");
                conn->close();
                return;
            }

            // 通知Lobby服加载玩家游戏对象
            if (!lobbyAgent->loadRole(sid, roleId, gateId)) {
                // 等待1秒后重新查询
                sleep(1); // 1秒后重试
            }

            cache = g_RedisPoolManager.getCoreCache()->take();
            if (!cache) {
                ERROR_LOG("GatewayHandler::authHandle -- reconnect to cache failed\n");
                conn->close();
                return;
            }
        } else {
            // 进游戏失败，这种情况可能是服务器负载高导致的，此时不清Session会好点
            ERROR_LOG("GatewayHandler::authHandle -- init role failed\n");
            conn->close();
            return;
        }
    }

    // 在登记到已连接表之前，调用一次SET_SESSION_EXPIRE_CMD，确保session没有过期
    switch (RedisUtils::SetSessionTTL(cache, userId, gToken)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GatewayHandler::authHandle -- user[%d] refresh session failed for db error\n", userId);
            conn->close();
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- user[%d] refresh session failed, gToken %s\n", userId, gToken.c_str());
            conn->close();
            return;
        }
    }

    assert(!g_GatewayObjectManager.hasGatewayObject(userId));

    // 在登记到已连接表之前，需要再判断一次是否断线（由于进行过能导致协程切换的redis操作）
    if (!conn->isOpen()) {
        ERROR_LOG("GatewayHandler::authHandle -- user %d disconnected when creating gateway object\n", userId);

        // 清理session（由于网络波动断线需要清session，不然玩家会一分钟登录不了）
        if (RedisUtils::RemoveSession(cache, userId, gToken) == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GatewayObject::stop -- user[%d] remove session failed", userId);
            return;
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
        return;
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    // 将网关对象登记到已连接表，登记完成后，游戏对象和客户端就能通过网关对象转发消息了
    g_GatewayObjectManager.addConnectedGatewayObject(obj);
    obj->start();

    // 通知游戏对象发开始游戏所需数据给客户端
    obj->enterGame();
}

void GatewayHandler::bypassHandle(int32_t type, uint16_t tag, std::shared_ptr<std::string> rawMsg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    //ERROR_LOG("GatewayHandler::bypassHandle msg:%d\n", type);

    std::shared_ptr<GatewayObject> obj = g_GatewayObjectManager.getConnectedGatewayObject(conn);
    if (!obj) {
        ERROR_LOG("GatewayHandler::bypassHandle -- gateway object not found, msg:%d\n", type);
        return;
    }

    obj->forwardIn(type, tag, rawMsg);
}

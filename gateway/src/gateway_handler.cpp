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
#include "gateway_center.h"
#include "share/const.h"
#include "string_utils.h"

#include "game.pb.h"

using namespace wukong;

void GatewayHandler::registerMessages(corpc::TcpMessageServer *server) {
    server->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false, std::bind(&GatewayHandler::connectHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    server->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, true, std::bind(&GatewayHandler::closeHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    server->registerMessage(CORPC_MSG_TYPE_BANNED, nullptr, true, std::bind(&GatewayHandler::banHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    server->registerMessage(C2S_MESSAGE_ID_AUTH, new pb::AuthRequest, true, std::bind(&GatewayHandler::authHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    server->setOtherMessageHandle(std::bind(&GatewayHandler::bypassHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void GatewayHandler::connectHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    // 登记未认证连接
    DEBUG_LOG("GatewayHandler::connectHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());

    if (_manager->isShutdown()) {
        conn->close();
    } else {
        if (conn->isOpen()) {
            _manager->addUnauthConn(conn);
        } else {
            DEBUG_LOG("GatewayHandler::connectHandle -- conn %d[%d] is not open\n", conn.get(), conn->getfd());
        }
    }
}

void GatewayHandler::closeHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    DEBUG_LOG("GatewayHandler::closeHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());
    assert(!conn->isOpen());
    if (_manager->isUnauth(conn)) {
        _manager->removeUnauthConn(conn);
    } else if (!_manager->tryMoveToDisconnectedLink(conn)) {
        // 注意：出现这种情况的原因是在认证过程中断线
        ERROR_LOG("GatewayHandler::closeHandle -- connection close when processing authenticated\n");
    }
}

void GatewayHandler::banHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    DEBUG_LOG("GatewayHandler::banHandle -- msgType:%d\n", type);
    // 向客户端发屏蔽消息
    std::shared_ptr<pb::BanResponse> response(new pb::BanResponse);
    response->set_msgid(type);

    conn->send(S2C_MESSAGE_ID_BAN, false, false, tag, response);
}

void GatewayHandler::authHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    DEBUG_LOG("GatewayHandler::authHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());
    if (_manager->isShutdown()) {
        ERROR_LOG("GatewayHandler::authHandle -- server shutdown\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    if (!conn->isOpen()) {
        ERROR_LOG("GatewayHandler::authHandle -- connection closed when begin auth\n");
        return;
    }

    // 确认连接是未认证连接（不允许重复发认证消息）
    if (!_manager->isUnauth(conn)) {
        ERROR_LOG("GatewayHandler::authHandle -- not an unauth connection\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    _manager->removeUnauthConn(conn);
    
    pb::AuthRequest *request = static_cast<pb::AuthRequest*>(msg.get());
    UserId userId = request->userid();

    // 若本地网关对象存在，直接跟本地网关对象的gToken比较，不需访问redis
    int ret = _manager->tryChangeGatewayObjectConn(userId, request->token(), conn);
    if (ret == -1) {
        ERROR_LOG("GatewayHandler::authHandle -- reconnect token not match\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    // 设置通讯密钥
    // TODO: 密钥应该通过RSA加密，需要解码密钥才能使用
    std::shared_ptr<Crypter> crypter(new SimpleXORCrypter(request->cipher()));
    conn->setCrypter(crypter);

    if (ret == 1) {
        // 在新连接下补发未收到的消息
        conn->scrapMessages(request->recvserial());
        conn->resend();
        return;
    }

    // 获取session比对gateId和gtoken
    redisContext *cache = g_GatewayCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("GatewayHandler::authHandle -- connect to cache failed\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    // 用lua脚本校验session信息——进行服务器号和token比较，设置超时时间，返回roleId
    redisReply *reply;
    if (g_GatewayCenter.checkSessionSha1().empty()) {
        reply = (redisReply *)redisCommand(cache, "EVAL %s 1 Session:%d %d %s %d", CHECK_SESSION_CMD, userId, _manager->getId(), request->token().c_str(), 60);
    } else {
        reply = (redisReply *)redisCommand(cache, "EVALSHA %s 1 Session:%d %d %s %d", g_GatewayCenter.checkSessionSha1(), userId, _manager->getId(), request->token().c_str(), 60);
    }
    
    if (!reply) {
        g_GatewayCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("GatewayHandler::authHandle -- check session failed for db error\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    RoleId roleId = reply->integer;
    if (reply->type != REDIS_REPLY_INTEGER || roleId == 0) {
        freeReplyObject(reply);
        g_GatewayCenter.getCachePool()->proxy.put(cache, false);
        DEBUG_LOG("GatewayHandler::authHandle -- check session failed\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    freeReplyObject(reply);

    // 由于协程存在穿插执行的情况，程序执行到这里时可能已经产生了网关对象，在创建网关对象前应校验一下
    if (_manager->hasGatewayObject(userId)) {
        g_GatewayCenter.getCachePool()->proxy.put(cache, false);
        ERROR_LOG("GatewayHandler::authHandle -- user %d gateway object already exist\n", userId);
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    // 若连接已断线，则不再继续创建网关对象（注意：此时需等session过期后才允许重新登录游戏（1分钟））
    if (!conn->isOpen()) {
        g_GatewayCenter.getCachePool()->proxy.put(cache, false);
        ERROR_LOG("GatewayHandler::authHandle -- user %d disconnected before create gateway object\n", userId);
        return;
    }

    // 创建玩家网关对象
    std::shared_ptr<GatewayObject> obj = std::make_shared<GatewayObject>(userId, roleId, request->token(), conn, _manager);

    // 查询玩家角色游戏对象所在（cache中key为location:<roleId>，值为<server type>|<server id>，由游戏对象负责维持心跳）
    //    若查到，设置玩家网关对象中的gameServerStub
    //    若没有查到，分配一个大厅服，并通知大厅服加载玩家游戏对象
    //       若返回失败，则登录失败
    reply = (redisReply *)redisCommand(cache, "HMGET location:%d lToken loc", roleId);
    if (!reply) {
        g_GatewayCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("GatewayHandler::authHandle -- get location failed for db error\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    if (reply->type != REDIS_REPLY_ARRAY || reply->elements != 2) {
        freeReplyObject(reply);
        g_GatewayCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("GatewayHandler::authHandle -- get location failed for invalid data type\n");
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    if (reply->element[0]->type == REDIS_REPLY_STRING) {
        assert(reply->element[1]->type == REDIS_REPLY_STRING);
        // 这里不再向游戏对象所在服发RPC（极端情况是游戏对象刚巧销毁导致网关对象指向了不存在的游戏对象的游戏服务器，网关对象一段时间没有收到游戏对象心跳后销毁）
        uint32_t lToken = atoi(reply->element[0]->str);
        std::string location = reply->element[1]->str;
        std::vector<std::string> output;
        StringUtils::split(location, "|", output);
        if (output.size() != 2) {
            freeReplyObject(reply);
            g_GatewayCenter.getCachePool()->proxy.put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- get location failed for invalid data\n");
            if (conn->isOpen()) {
                conn->close();
            }
            return;
        }

        GameServerType stype = std::stoi(output[0]);
        ServerId sid = std::stoi(output[1]);
        if (!obj->setGameServerStub(stype, sid)) {
            freeReplyObject(reply);
            g_GatewayCenter.getCachePool()->proxy.put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- set game server stub failed\n");
            if (conn->isOpen()) {
                conn->close();
            }
            return;
        }
        obj->setLToken(lToken);
    } else {
        assert(reply->element[0]->type == REDIS_REPLY_NIL);
        assert(reply->element[1]->type == REDIS_REPLY_NIL);

        // 分配一个Lobby服，并通知Lobby服加载玩家游戏对象
        ServerId sid = 0;
        if (!g_GatewayCenter.randomLobbyServer(sid)) {
            freeReplyObject(reply);
            g_GatewayCenter.getCachePool()->proxy.put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- random lobby server failed\n");
            if (conn->isOpen()) {
                conn->close();
            }
            return;
        }

        if (!obj->setGameServerStub(GAME_SERVER_TYPE_LOBBY, sid)) {
            freeReplyObject(reply);
            g_GatewayCenter.getCachePool()->proxy.put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- set lobby server stub failed\n");
            if (conn->isOpen()) {
                conn->close();
            }
            return;
        }

        // 通知Lobby服加载玩家游戏对象
        uint32_t lToken = g_LobbyClient.initRole(sid, userId, roleId, _manager->getId());
        if (lToken == 0) {
            freeReplyObject(reply);
            g_GatewayCenter.getCachePool()->proxy.put(cache, false);
            ERROR_LOG("GatewayHandler::authHandle -- load role failed\n");
            if (conn->isOpen()) {
                conn->close();
            }
            return;
        }
        obj->setLToken(lToken);
    }

    freeReplyObject(reply);
    g_GatewayCenter.getCachePool()->proxy.put(cache, false);

    // 由于进行过能导致协程切换的redis操作，这里需要再次确认中途没有网关对象登记到已连接表
    if (_manager->hasGatewayObject(userId)) {
        ERROR_LOG("GatewayHandler::authHandle -- user %d gateway object already exist after get role %d location\n", userId, roleId);
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    // 由于进行过能导致协程切换的redis操作，在登记到已连接表之前，需要再判断一次是否断线
    if (!conn->isOpen()) {
        ERROR_LOG("GatewayHandler::authHandle -- user %d disconnected when creating gateway object\n", userId);
        return;
    }

    // 将网关对象登记到已连接表，登记完成后，游戏对象和客户端就能通过网关对象转发消息了
    _manager->addConnectedGatewayObject(obj);
    obj->start();

    // 通知游戏对象发开始游戏所需数据给客户端
    obj->enterGame();
}


void GatewayHandler::bypassHandle(int16_t type, uint16_t tag, std::shared_ptr<std::string> rawMsg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    DEBUG_LOG("GatewayHandler::bypassHandle -- msgType:%d\n", type);
    std::shared_ptr<GatewayObject> obj = _manager->getConnectedGatewayObject(conn);
    if (!obj) {
        ERROR_LOG("GatewayHandler::bypassHandle -- gateway object not found\n");
        return;
    }

    obj->forwardIn(type, tag, rawMsg);
}

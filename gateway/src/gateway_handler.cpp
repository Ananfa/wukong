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

#include "game.pb.h"

using namespace wukong;

void GatewayHandler::registerMessages(corpc::TcpMessageServer *server) {
    server->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false, std::bind(&GatewayHandler::connectHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    server->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, true, std::bind(&GatewayHandler::closeHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    server->registerMessage(CORPC_MSG_TYPE_BANNED, nullptr, true, std::bind(&GatewayHandler::banHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    server->registerMessage(MESSAGE_ID_AUTH_REQ, new pb::AuthRequest, true, std::bind(&GatewayHandler::authHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

    server->setOtherMessageHandle(std::bind(&GatewayHandler::bypassHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
}

void GatewayHandler::connectHandle(int16_t type, uint8_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    // 登记未认证连接
    DEBUG_LOG("GatewayHandler::connectHandle - %d[%d]\n", conn.get(), conn->getfd());

    if (_manager->isShutdown()) {
        conn->close();
    } else {
        if (conn->isOpen()) {
            _manager->addUnauthConn(conn);
        } else {
            DEBUG_LOG("GatewayHandler::connectHandle - conn %d[%d] is not open\n", conn.get(), conn->getfd());
        }
    }
}

void GatewayHandler::closeHandle(int16_t type, uint8_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    assert(!conn->isOpen());
    if (_manager->isUnauth(conn)) {
        _manager->removeUnauthConn(conn);
    } else if (!_manager->tryMoveToDisconnectedLink(conn)) {
        // 注意：出现这种情况的原因是在认证过程中断线
        ERROR_LOG("GatewayHandler::closeHandle -- connection close when processing authenticated\n");
    }
}

void GatewayHandler::banHandle(int16_t type, uint8_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
    // TODO: 向客户端发屏蔽消息
}

void GatewayHandler::authHandle(int16_t type, uint8_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn) {
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

    // 若本地路由对象存在，直接跟本地路由对象的gToken比较，不需访问redis
    int ret = _manager->tryChangeRouteObjectConn(userId, request->token(), conn);
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
        reply = (redisReply *)redisCommand(cache, "eval %s 1 session:%d %d %s %d", CHECK_SESSION_CMD, userId, _manager->getId(), request->token().c_str(), 60);
    } else {
        reply = (redisReply *)redisCommand(cache, "eval %s 1 session:%d %d %s %d", g_GatewayCenter.checkSessionSha1(), userId, _manager->getId(), request->token().c_str(), 60);
    }
    
    if (!reply) {
        ERROR_LOG("GatewayHandler::authHandle -- check session failed for db error\n");
        g_GatewayCenter.getCachePool()->proxy.put(cache, true);
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    RoleId roleId = reply->integer;
    if (reply->type != REDIS_REPLY_INTEGER || roleId == 0) {
        DEBUG_LOG("GatewayHandler::authHandle -- check session failed\n");
        freeReplyObject(reply);
        g_GatewayCenter.getCachePool()->proxy.put(cache, false);
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    freeReplyObject(reply);

    // 由于协程存在穿插执行的情况，程序执行到这里时可能已经产生了路由对象，在创建路由对象前应校验一下
    if (_manager->hasRouteObject(userId)) {
        ERROR_LOG("GatewayHandler::authHandle -- user %d route object already exist\n", userId);
        g_GatewayCenter.getCachePool()->proxy.put(cache, false);
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    // 若连接已断线，则不再继续创建路由对象（注意：此时需等session过期后才允许重新登录游戏（1分钟））
    if (!conn->isOpen()) {
        ERROR_LOG("GatewayHandler::authHandle -- user %d disconnected before create route object\n", userId);
        g_GatewayCenter.getCachePool()->proxy.put(cache, false);
        return;
    }

    // 创建玩家路由对象
    std::shared_ptr<RouteObject> ro = std::make_shared<RouteObject>(userId, roleId, request->token(), conn, _manager);

    // TODO: 查询玩家角色游戏对象所在（cache中key为location:<roleId>，值为<server type>|<server id>，由游戏对象负责维持心跳）
    //       若查到，向目标游戏服发RPC询问是否存在(带超时)，避免假游戏对象
    //          若存在，设置玩家路由对象中的targetServerStub
    //          若超时或不存在，出现异常，登录失败
    //       若没有，分配一个大厅服，并通知大厅服加载玩家游戏对象
    //          若返回失败，则登录失败
    reply = (redisReply *)redisCommand(cache, "GET location:%d", roleId);
    if (!reply) {
        ERROR_LOG("GatewayHandler::authHandle -- get location failed for db error\n");
        g_GatewayCenter.getCachePool()->proxy.put(cache, true);
        return;
    }

    if (reply->type == REDIS_REPLY_STRING) {
        // 向目标游戏服发RPC询问是否存在(带超时)，避免假游戏对象
    } else if (reply->type == REDIS_REPLY_NIL) {

    }

    // TODO:.....


    g_GatewayCenter.getCachePool()->proxy.put(cache, false);

    // 由于进行过能导致协程切换的redis操作，这里需要再次确认中途没有路由对象登记到已连接表
    if (_manager->hasRouteObject(userId)) {
        ERROR_LOG("GatewayHandler::authHandle -- user %d route object already exist after get role %d location\n", userId, roleId);
        if (conn->isOpen()) {
            conn->close();
        }
        return;
    }

    // 由于进行过能导致协程切换的redis操作，在登记到已连接表之前，需要再判断一次是否断线
    if (!conn->isOpen()) {
        ERROR_LOG("GatewayHandler::authHandle -- user %d disconnected when creating route object\n", userId);
        return;
    }

    // 将路由对象登记到已连接表，登记完成后，游戏对象和客户端就能通过路由对象转发消息了
    _manager->addConnectedRouteObject(ro);
    ro->open();

    // TODO: 通知游戏对象发开始游戏所需数据给客户端
    
}


void GatewayHandler::bypassHandle(int16_t type, uint8_t tag, std::shared_ptr<std::string> rawMsg, std::shared_ptr<corpc::MessageServer::Connection> conn) {

}

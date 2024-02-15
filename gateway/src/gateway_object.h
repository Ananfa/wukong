/*
 * Created by Xianke Liu on 2021/1/7.
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

#ifndef wukong_gateway_object_h
#define wukong_gateway_object_h

#include "corpc_message_server.h"
#include "corpc_cond.h"
#include "share/define.h"
#include "game_service.pb.h"

using namespace corpc;

namespace wukong {
    class GatewayObject;
    class GatewayObjectManager;

    struct GatewayObjectRoutineArg {
        std::shared_ptr<GatewayObject> obj;
    };

    class GatewayObject: public std::enable_shared_from_this<GatewayObject> {
    public:
        GatewayObject(UserId userId, RoleId roleId, const std::string &gToken, std::shared_ptr<MessageServer::Connection> &conn, GatewayObjectManager *manager): userId_(userId), roleId_(roleId), gToken_(gToken), conn_(conn), manager_(manager), running_(false) {}

        std::shared_ptr<MessageServer::Connection> &getConn() { return conn_; }
        std::shared_ptr<pb::GameService_Stub> &getGameServerStub() { return gameServerStub_; }
        const std::string &getGToken() { return gToken_; }
        void setLToken(const std::string &lToken) { lToken_ = lToken; }
        const std::string &getLToken() { return lToken_; }
        UserId getUserId() { return userId_; }
        RoleId getRoleId() { return roleId_; }

        void setConn(std::shared_ptr<MessageServer::Connection> &conn) { conn_ = conn; }
        bool setGameServerStub(GameServerType gsType, ServerId sid);

        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳

        /* 业务逻辑 */
        void forwardIn(int16_t type, uint16_t tag, std::shared_ptr<std::string> &rawMsg);
        void enterGame(); // 通知进入游戏

    private:
        static void *heartbeatRoutine( void *arg );  // 心跳协程，周期对session重设超时时间，心跳失败时需通知Manager销毁网关对象

    private:
        std::shared_ptr<MessageServer::Connection> conn_; // 客户端连接
        std::shared_ptr<pb::GameService_Stub> gameServerStub_; // 游戏对象所在服务器stub
        GameServerType gameServerType_; // 游戏对象所在服务器类型
        ServerId gameServerId_; // 游戏对象所在服务器id
        std::string gToken_; // 断线重连校验身份用
        std::string lToken_; // 游戏对象唯一标识
        UserId userId_;
        RoleId roleId_;

        bool running_;

        uint64_t gameObjectHeartbeatExpire_; // 游戏对象心跳超时时间
        Cond cond_;

        GatewayObjectManager *manager_; // 关联的manager

    public:
        friend class InnerGatewayServiceImpl;
    };
}

#endif /* wukong_gateway_object_h */

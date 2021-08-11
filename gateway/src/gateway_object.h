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

#ifndef gateway_object_h
#define gateway_object_h

#include "corpc_message_server.h"
#include "corpc_cond.h"
#include "share/define.h"
#include "game_service.pb.h"

using namespace corpc;

namespace wukong {
    class GatewayObject;
    class GatewayManager;

    struct GatewayObjectRoutineArg {
        std::shared_ptr<GatewayObject> obj;
    };

    class GatewayObject: public std::enable_shared_from_this<GatewayObject> {
    public:
        GatewayObject(UserId userId, RoleId roleId, const std::string &gToken, std::shared_ptr<MessageServer::Connection> &conn, GatewayManager *manager): _userId(userId), _roleId(roleId), _gToken(gToken), _conn(conn), _manager(manager), _running(false) {}

        std::shared_ptr<MessageServer::Connection> &getConn() { return _conn; }
        std::shared_ptr<pb::GameService_Stub> &getGameServerStub() { return _gameServerStub; }
        const std::string &getGToken() { return _gToken; }
        void setLToken(uint32_t lToken) { _lToken = lToken; }
        uint32_t &getLToken() { return _lToken; }
        UserId getUserId() { return _userId; }
        RoleId getRoleId() { return _roleId; }

        void setConn(std::shared_ptr<MessageServer::Connection> &conn) { _conn = conn; }
        bool setGameServerStub(GameServerType gsType, ServerId sid);

        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳

        /* 业务逻辑 */
        void forwardIn(int16_t type, uint16_t tag, std::shared_ptr<std::string> &rawMsg);
        void enterGame(); // 通知进入游戏

    private:
        static void *heartbeatRoutine( void *arg );  // 心跳协程，周期对session重设超时时间，心跳失败时需通知Manager销毁网关对象

    private:
        std::shared_ptr<MessageServer::Connection> _conn; // 客户端连接
        std::shared_ptr<pb::GameService_Stub> _gameServerStub; // 游戏对象所在服务器stub
        GameServerType _gameServerType; // 游戏对象所在服务器类型
        ServerId _gameServerId; // 游戏对象所在服务器id
        std::string _gToken; // 断线重连校验身份用
        uint32_t _lToken; // 游戏对象唯一标识
        UserId _userId;
        RoleId _roleId;

        bool _running;

        uint64_t _gameObjectHeartbeatExpire; // 游戏对象心跳超时时间
        Cond _cond;

        GatewayManager *_manager; // 关联的manager

    public:
        friend class GatewayServiceImpl;
    };
}

#endif /* gateway_object_h */

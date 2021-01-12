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

#ifndef route_object_h
#define route_object_h

namespace wukong {
    class RouteObject;
    class GatewayManager;

    struct RouteObjectHeartbeatInfo {
        std::shared_ptr<RouteObject> ro;
    };

    class RouteObject: public std::enable_shared_from_this<RouteObject> {
    public:
        RouteObject(UserId userId, RoleId roleId, const std::string &token, std::shared_ptr<MessageServer::Connection> &conn, GatewayManager *manager): _userId(userId), _roleId(roleId), _token(token), _conn(conn), _manager(manager), _running(false) {}

        std::shared_ptr<MessageServer::Connection> &getConn() { return _conn; }
        std::shared_ptr<pb::GameService_Stub> &getTargetServerStub() { return _targetServerStub; }
        const std::string &getToken() { return _token; }
        UserId getUserId() { return _userId; }
        RoleId getRoleId() { return _roleId; }

        void setConn(std::shared_ptr<MessageServer::Connection> &conn) { _conn = conn; }
        void setTargetServerStub(std::shared_ptr<pb::GameService_Stub> &stub) { _targetServerStub = stub; }

        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳

    private:
        static void *heartbeatRoutine( void *arg );  // 心跳协程，周期对session重设超时时间，心跳失败时需通知Manager销毁路由对象

    private:
        std::shared_ptr<MessageServer::Connection> _conn; // 客户端连接
        std::shared_ptr<pb::GameService_Stub> _targetServerStub; // 游戏对象所在服务器
        std::string _token; // 断线重连校验身份用
        UserId _userId;
        RoleId _roleId;

        bool _running;

        GatewayManager *_manager; // 关联的manager
    };

#endif /* route_object_h */

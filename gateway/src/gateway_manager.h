/*
 * Created by Xianke Liu on 2020/12/15.
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

#ifndef gateway_manager_h
#define gateway_manager_h

#include "corpc_message_server.h"
#include "corpc_redis.h"
#include "timelink.h"
#include "route_object.h"
#include "share/define.h"
#include "game_service.pb.h"

using namespace corpc;

namespace wukong {
    typedef TimeLink<MessageServer::Connection> MessageConnectionTimeLink;
    typedef TimeLink<RouteObject> RouteObjectTimeLink;

    class GatewayManager {
    public:
        GatewayManager(ServerId id):_id(id), _shutdown(false) {}

        void init();

        ServerId getId() { return _id; }

        void shutdown() { _shutdown = true; }
        bool isShutdown() { return _shutdown; }

        void addUnauthConn(std::shared_ptr<MessageServer::Connection>& conn);
        void removeUnauthConn(std::shared_ptr<MessageServer::Connection>& conn);
        bool isUnauth(std::shared_ptr<MessageServer::Connection>& conn);
        void clearUnauth();
        size_t unauthSize() const { return _unauthNodeMap.size(); }

        bool hasRouteObject(UserId userId); // 判断玩家路由对象是否存在（包括已连接及断线中）
        bool removeRouteObject(UserId userId); // 删除玩家路由对象（包括已连接及断线中）

        // 获取已连接的路由对象
        std::shared_ptr<RouteObject> getConnectedRouteObject(UserId userId);
        std::shared_ptr<RouteObject> getConnectedRouteObject(std::shared_ptr<MessageServer::Connection> &conn);

        void addConnectedRouteObject(std::shared_ptr<RouteObject> &ro);

        // tryChangeRouteObjectConn方法当token一致时更新路由对象（包括已连接及断线中）的客户端连接
        // 返回：-1.token不一致更换失败 0.路由对象不存在 1.更换成功
        int tryChangeRouteObjectConn(UserId userId, const std::string &token, std::shared_ptr<MessageServer::Connection> &newConn);

        // tryMoveToDisconnectedLink将路由对象从已连接表转移到断线表，若没有找到返回false
        bool tryMoveToDisconnectedLink(std::shared_ptr<MessageServer::Connection> &conn);

    private:
        static void *clearExpiredUnauthRoutine( void *arg );  // 清理过时未认证连接
        static void *clearExpiredDisconnectedRoutine( void *arg ); // 清理过时断线路由对象

    private:
        ServerId _id;       // gateway服务号
        bool _shutdown;

        // 未认证连接相关数据结构
        MessageConnectionTimeLink _unauthLink;
        std::map<MessageServer::Connection*, MessageConnectionTimeLink::Node*> _unauthNodeMap;
    
        // 断线中的路由对象相关数据结构（等待过期清理或者断线重连）
        RouteObjectTimeLink _disconnectedLink;
        std::map<UserId, RouteObjectTimeLink::Node*> _disconnectedNodeMap;

        // 正常路由对象列表
        std::map<UserId, std::shared_ptr<RouteObject>> _userId2RouteObjectMap;
        std::map<MessageServer::Connection*, std::shared_ptr<RouteObject>> _connection2RouteObjectMap;
    };

}

#endif /* gateway_manager_h */

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

#ifndef wukong_gateway_object_manager_h
#define wukong_gateway_object_manager_h

#include "corpc_message_server.h"
#include "corpc_redis.h"
#include "timelink.h"
#include "gateway_object.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    typedef TimeLink<MessageServer::Connection> MessageConnectionTimeLink;
    typedef TimeLink<GatewayObject> GatewayObjectTimeLink;

    class GatewayObjectManager {
    public:
        GatewayObjectManager(ServerId id):_id(id), _shutdown(false) {}

        void init();

        ServerId getId() { return _id; }

        void shutdown();
        bool isShutdown() { return _shutdown; }

        void addUnauthConn(std::shared_ptr<MessageServer::Connection>& conn);
        void removeUnauthConn(std::shared_ptr<MessageServer::Connection>& conn);
        bool isUnauth(std::shared_ptr<MessageServer::Connection>& conn);
        void clearUnauth();
        size_t unauthSize() const { return _unauthNodeMap.size(); }

        bool hasGatewayObject(UserId userId); // 判断玩家网关对象是否存在（包括已连接及断线中）
        bool removeGatewayObject(UserId userId); // 删除玩家网关对象（包括已连接及断线中）
        size_t getGatewayObjectNum(); // 获取当前网关对象数
        void clearGatewayObject(); // 清理所有网关对象

        std::shared_ptr<GatewayObject> getGatewayObject(UserId userId); // 获取玩家网关对象（包括已连接及断线中）

        // 获取已连接的网关对象
        std::shared_ptr<GatewayObject> getConnectedGatewayObject(UserId userId);
        std::shared_ptr<GatewayObject> getConnectedGatewayObject(std::shared_ptr<MessageServer::Connection> &conn);
        void traverseConnectedGatewayObject(std::function<bool(std::shared_ptr<GatewayObject>&)> handle); // 注意：handle方法中不能有协程切换产生

        void addConnectedGatewayObject(std::shared_ptr<GatewayObject> &obj);

        // tryChangeGatewayObjectConn方法当token一致时更新网关对象（包括已连接及断线中）的客户端连接
        // 返回：-1.token不一致更换失败 0.网关对象不存在 1.更换成功
        int tryChangeGatewayObjectConn(UserId userId, const std::string &token, std::shared_ptr<MessageServer::Connection> &newConn);

        // tryMoveToDisconnectedLink将网关对象从已连接表转移到断线表，若没有找到返回false
        bool tryMoveToDisconnectedLink(std::shared_ptr<MessageServer::Connection> &conn);

    private:
        static void *clearExpiredUnauthRoutine( void *arg );  // 清理过时未认证连接
        static void *clearExpiredDisconnectedRoutine( void *arg ); // 清理过时断线网关对象

    private:
        ServerId _id;       // gateway服务号
        bool _shutdown;

        // 未认证连接相关数据结构
        MessageConnectionTimeLink _unauthLink;
        std::map<MessageServer::Connection*, MessageConnectionTimeLink::Node*> _unauthNodeMap;
    
        // 断线中的网关对象相关数据结构（等待过期清理或者断线重连）
        GatewayObjectTimeLink _disconnectedLink;
        std::map<UserId, GatewayObjectTimeLink::Node*> _disconnectedNodeMap;

        // 正常网关对象列表
        std::map<UserId, std::shared_ptr<GatewayObject>> _userId2GatewayObjectMap;
        std::map<MessageServer::Connection*, std::shared_ptr<GatewayObject>> _connection2GatewayObjectMap;
    };

}

#endif /* wukong_gateway_object_manager_h */

/*
 * Created by Xianke Liu on 2024/7/6.
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

#ifndef wukong_server_manager_h
#define wukong_server_manager_h

#include "corpc_message_terminal.h"
#include "server_object.h"
#include "timelink.h"
#include "define.h"

using namespace corpc;

namespace wukong {
    typedef TimeLink<MessageTerminal::Connection> MessageConnectionTimeLink;
    typedef TimeLink<ServerObject> ServerObjectTimeLink;

    // 注意： ServerManager非线程安全，应在同一线程中管理
    class ServerManager {
    public:
        static ServerManager& Instance() {
            static ServerManager instance;
            return instance;
        }

        bool start();   // 在管理线程中调用一次

        void addUnaccessConn(std::shared_ptr<MessageTerminal::Connection>& conn);
        void removeUnaccessConn(std::shared_ptr<MessageTerminal::Connection>& conn);
        bool isUnaccess(std::shared_ptr<MessageTerminal::Connection>& conn);
        void clearUnaccess();

        bool hasServerObject(ServerType stype, ServerId sid); // 判断服务对象是否存在（包括已连接及断线中）
        bool removeServerObject(ServerType stype, ServerId sid); // 删除服务对象（包括已连接及断线中）
        //bool isDisconnectedObject(ServerType stype, ServerId sid); // 是否断线中

        std::shared_ptr<ServerObject> getServerObject(ServerType stype, ServerId sid); // 获取服务对象（包括已连接及断线中）
        std::shared_ptr<ServerObject> getConnectedServerObject(std::shared_ptr<MessageTerminal::Connection> &conn);

        const std::map<ServerId, std::shared_ptr<ServerObject>>& getServerObjects(ServerType stype);

        void moveToDisconnected(std::shared_ptr<ServerObject> &obj);
        void addConnectedServer(std::shared_ptr<ServerObject> &obj);

    private:
        static void *clearExpiredUnaccessRoutine( void *arg );  // 清理过时未认证连接
        static void *clearExpiredDisconnectedRoutine( void *arg ); // 清理过时断线服务对象

    private:
        bool started;

        // 未认证连接相关数据结构
        MessageConnectionTimeLink unaccessLink_;
        std::map<MessageTerminal::Connection*, MessageConnectionTimeLink::Node*> unaccessNodeMap_;
    
        // 断线中的服务对象相关数据结构（等待过期清理或者断线重连）
        ServerObjectTimeLink disconnectedLink_;
        std::map<ServerType, std::map<ServerId, ServerObjectTimeLink::Node*>> disconnectedNodeMap_;

        // 正常服务对象列表
        std::map<ServerType, std::map<ServerId, std::shared_ptr<ServerObject>>> serverObjectMap_;
        std::map<MessageTerminal::Connection*, std::shared_ptr<ServerObject>> connection2ServerObjectMap_;

        std::map<ServerId, std::shared_ptr<ServerObject>> emptyServerObjectMap_; // 用于找不到时返回

    private:
        ServerManager(): started(false) {}                               // ctor hidden
        ~ServerManager() = default;                                      // destruct hidden
        ServerManager(ServerManager const&) = delete;                    // copy ctor delete
        ServerManager(ServerManager &&) = delete;                        // move ctor delete
        ServerManager& operator=(ServerManager const&) = delete;         // assign op. delete
        ServerManager& operator=(ServerManager &&) = delete;             // move assign op. delete
    };
}

#define g_ServerManager wukong::ServerManager::Instance()

#endif /* wukong_server_manager_h */

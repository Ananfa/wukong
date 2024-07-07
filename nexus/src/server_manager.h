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

    class ServerManager {
    public:
        static ServerManager& Instance() {
            static ServerManager instance;
            return instance;
        }

        void addUnknownConn(std::shared_ptr<MessageTerminal::Connection>& conn);
        void removeUnknownConn(std::shared_ptr<MessageTerminal::Connection>& conn);
        bool isUnknown(std::shared_ptr<MessageTerminal::Connection>& conn);
        void clearUnknown();

        bool tryMoveToDisconnectedLink(std::shared_ptr<MessageTerminal::Connection> &conn);

    private:
        // 未认证连接相关数据结构
        MessageConnectionTimeLink unknownLink_;
        std::map<MessageTerminal::Connection*, MessageConnectionTimeLink::Node*> unknownNodeMap_;
    
        // 断线中的服务对象相关数据结构（等待过期清理或者断线重连）
        ServerObjectTimeLink disconnectedLink_;
        std::map<ServerType, std::map<ServerId, ServerObjectTimeLink::Node*>> disconnectedNodeMap_;

        // 正常服务对象列表
        std::map<ServerType, std::map<ServerId, std::shared_ptr<ServerObject>>> serverObjectMap_;
        std::map<MessageTerminal::Connection*, std::shared_ptr<ServerObject>> connection2ServerObjectMap_;

    private:
        ServerManager() = default;                                       // ctor hidden
        ~ServerManager() = default;                                      // destruct hidden
        ServerManager(ServerManager const&) = delete;                    // copy ctor delete
        ServerManager(ServerManager &&) = delete;                        // move ctor delete
        ServerManager& operator=(ServerManager const&) = delete;         // assign op. delete
        ServerManager& operator=(ServerManager &&) = delete;             // move assign op. delete
    };
}

#define g_ServerManager wukong::ServerManager::Instance()

#endif /* wukong_server_manager_h */

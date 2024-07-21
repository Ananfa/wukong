/*
 * Created by Xianke Liu on 2021/6/8.
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

#ifndef wukong_gateway_server_h
#define wukong_gateway_server_h

#include "corpc_rpc_client.h"
#include "corpc_inner_rpc.h"
#include "corpc_message_server.h"
#include "game_client.h"
#include "share/define.h"
#include "gateway_service.pb.h"
#include <map>
#include <thread>
#include <functional>

using namespace corpc;

namespace wukong {
    // 单例模式实现
    class GatewayServer {
    public:
        static GatewayServer& Instance() {
            static GatewayServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        void enterZoo();

        static void gatewayThread(InnerRpcServer *server, IO *msg_io, ServerId gwid, uint16_t msgPort);

        static void banMsgHandle(corpc::MessageTerminal *terminal);

    private:
        bool inited_ = false;
        IO *io_ = nullptr;
        RpcClient *rpcClient_ = nullptr;

        std::vector<std::thread> threads_;

    private:
        GatewayServer() = default;                                  // ctor hidden
        GatewayServer(GatewayServer const&) = delete;               // copy ctor hidden
        GatewayServer(GatewayServer &&) = delete;                   // move ctor hidden
        GatewayServer& operator=(GatewayServer const&) = delete;    // assign op. hidden
        GatewayServer& operator=(GatewayServer &&) = delete;        // move assign op. hidden
        ~GatewayServer() = default;                                 // dtor hidden
    };

}

#define g_GatewayServer wukong::GatewayServer::Instance()

#endif /* wukong_gateway_server_h */

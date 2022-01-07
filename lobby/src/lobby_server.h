/*
 * Created by Xianke Liu on 2021/6/9.
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

#ifndef lobby_server_h
#define lobby_server_h

#include "corpc_rpc_client.h"
#include "corpc_inner_rpc.h"
#include "share/define.h"
#include "lobby_service.pb.h"
#include <map>
#include <thread>

using namespace corpc;

namespace wukong {

    // 单例模式实现
    class LobbyServer {
    private:
        bool _inited = false;
        IO *_io = nullptr;
        RpcClient *_rpcClient = nullptr;

        std::vector<std::thread> _threads;
    public:
        static LobbyServer& Instance() {
            static LobbyServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

        IO *getIO() { return _io; }
        RpcClient *getRpcClient() { return _rpcClient; }
        
    private:
        void enterZoo();

        static void lobbyThread(InnerRpcServer *server, ServerId lbid);

    private:
        LobbyServer() = default;                                // ctor hidden
        LobbyServer(LobbyServer const&) = delete;               // copy ctor hidden
        LobbyServer(LobbyServer &&) = delete;                   // move ctor hidden
        LobbyServer& operator=(LobbyServer const&) = delete;    // assign op. hidden
        LobbyServer& operator=(LobbyServer &&) = delete;        // move assign op. hidden
        ~LobbyServer() = default;                               // dtor hidden
    };

    #define g_LobbyServer LobbyServer::Instance()
}

#endif /* lobby_server_h */

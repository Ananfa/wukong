/*
 * Created by Xianke Liu on 2025/5/21.
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

#ifndef wukong_chat_server_h
#define wukong_chat_server_h

#include "corpc_rpc_client.h"
#include "share/define.h"
#include "chat_service.pb.h"
#include <map>
#include <thread>

using namespace corpc;

namespace wukong {

    // 单例模式实现
    class ChatServer {
    public:
        static ChatServer& Instance() {
            static ChatServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        bool inited_ = false;
        IO *io_ = nullptr;
        RpcClient *rpcClient_ = nullptr;
        
    private:
        ChatServer() = default;                               // ctor hidden
        ChatServer(ChatServer const&) = delete;               // copy ctor hidden
        ChatServer(ChatServer &&) = delete;                   // move ctor hidden
        ChatServer& operator=(ChatServer const&) = delete;    // assign op. hidden
        ChatServer& operator=(ChatServer &&) = delete;        // move assign op. hidden
        ~ChatServer() = default;                              // dtor hidden
    };

}

#define g_ChatServer wukong::ChatServer::Instance()

#endif /* wukong_chat_server_h */

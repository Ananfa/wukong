/*
 * Created by Xianke Liu on 2021/6/7.
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

#ifndef wukong_login_server_h
#define wukong_login_server_h

#include "corpc_rpc_client.h"
#include "http_server.h"

namespace wukong {

    // 单例模式实现
    class LoginServer {
    private:
        bool inited_ = false;
        IO *io_ = nullptr;
        RpcClient *rpcClient_ = nullptr;
        HttpServer *_httpServer = nullptr;

    public:
        static LoginServer& Instance() {
            static LoginServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        LoginServer() = default;                                // ctor hidden
        LoginServer(LoginServer const&) = delete;               // copy ctor hidden
        LoginServer(LoginServer &&) = delete;                   // move ctor hidden
        LoginServer& operator=(LoginServer const&) = delete;    // assign op. hidden
        LoginServer& operator=(LoginServer &&) = delete;        // move assign op. hidden
        ~LoginServer() = default;                               // dtor hidden
    };

}

#define g_LoginServer wukong::LoginServer::Instance()

#endif /* wukong_login_server_h */

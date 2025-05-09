/*
 * Created by Xianke Liu on 2025/5/7.
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

#ifndef wukong_opctrl_server_h
#define wukong_opctrl_server_h

#include "corpc_rpc_client.h"
#include "http_server.h"

namespace wukong {

    // 单例模式实现
    class OpctrlServer {
    private:
        bool inited_ = false;
        IO *io_ = nullptr;
        RpcClient *rpcClient_ = nullptr;
        HttpServer *_httpServer = nullptr;

    public:
        static OpctrlServer& Instance() {
            static OpctrlServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

        HttpServer *getHttpServer() { return _httpServer; }

    private:
        OpctrlServer() = default;                                 // ctor hidden
        OpctrlServer(OpctrlServer const&) = delete;               // copy ctor hidden
        OpctrlServer(OpctrlServer &&) = delete;                   // move ctor hidden
        OpctrlServer& operator=(OpctrlServer const&) = delete;    // assign op. hidden
        OpctrlServer& operator=(OpctrlServer &&) = delete;        // move assign op. hidden
        ~OpctrlServer() = default;                                // dtor hidden
    };

}

#define g_OpctrlServer wukong::OpctrlServer::Instance()

#endif /* wukong_opctrl_server_h */

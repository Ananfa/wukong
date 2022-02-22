/*
 * Created by Xianke Liu on 2022/1/12.
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

#ifndef scene_server_h
#define scene_server_h

#include "corpc_rpc_client.h"
#include "corpc_inner_rpc.h"
#include "share/define.h"
#include "scene_service.pb.h"
#include <map>
#include <thread>

using namespace corpc;

namespace wukong {

    // 单例模式实现
    class SceneServer {
    private:
        bool _inited = false;
        IO *_io = nullptr;
        RpcClient *_rpcClient = nullptr;

        std::vector<std::thread> _threads;
    public:
        static SceneServer& Instance() {
            static SceneServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        static void lobbyThread(InnerRpcServer *server, ServerId sid);

    private:
        SceneServer() = default;                                // ctor hidden
        SceneServer(SceneServer const&) = delete;               // copy ctor hidden
        SceneServer(SceneServer &&) = delete;                   // move ctor hidden
        SceneServer& operator=(SceneServer const&) = delete;    // assign op. hidden
        SceneServer& operator=(SceneServer &&) = delete;        // move assign op. hidden
        ~SceneServer() = default;                               // dtor hidden
    };

    #define g_SceneServer SceneServer::Instance()
}

#endif /* scene_server_h */

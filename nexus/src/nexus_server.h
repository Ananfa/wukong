/*
 * Created by Xianke Liu on 2024/7/5.
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

#ifndef wukong_nexus_server_h
#define wukong_nexus_server_h

#include "corpc_message_server.h"

using namespace corpc;

namespace wukong {
    // 单例模式实现
    class NexusServer {
    public:
        static NexusServer& Instance() {
            static NexusServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        bool inited_ = false;

    private:
        NexusServer() = default;                                // ctor hidden
        NexusServer(NexusServer const&) = delete;               // copy ctor hidden
        NexusServer(NexusServer &&) = delete;                   // move ctor hidden
        NexusServer& operator=(NexusServer const&) = delete;    // assign op. hidden
        NexusServer& operator=(NexusServer &&) = delete;        // move assign op. hidden
        ~NexusServer() = default;                               // dtor hidden
    };

}

#define g_NexusServer wukong::NexusServer::Instance()

#endif /* wukong_nexus_server_h */

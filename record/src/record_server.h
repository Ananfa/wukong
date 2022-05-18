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

#ifndef wukong_record_server_h
#define wukong_record_server_h

#include "corpc_rpc_client.h"
#include "corpc_inner_rpc.h"
#include "share/define.h"
#include "record_service.pb.h"
#include <map>
#include <thread>

using namespace corpc;

namespace wukong {

    // 单例模式实现
    class RecordServer {
    private:
        bool _inited = false;
        IO *_io = nullptr;

        std::vector<std::thread> _threads;
    public:
        static RecordServer& Instance() {
            static RecordServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        static void recordThread(InnerRpcServer *server, ServerId rcid);

    private:
        RecordServer() = default;                                   // ctor hidden
        RecordServer(RecordServer const&) = delete;                 // copy ctor hidden
        RecordServer(RecordServer &&) = delete;                     // move ctor hidden
        RecordServer& operator=(RecordServer const&) = delete;      // assign op. hidden
        RecordServer& operator=(RecordServer &&) = delete;          // move assign op. hidden
        ~RecordServer() = default;                                  // dtor hidden
    };

    #define g_RecordServer RecordServer::Instance()
}

#endif /* wukong_record_server_h */

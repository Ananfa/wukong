/*
 * Created by Xianke Liu on 2024/6/15.
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

#ifndef wukong_front_server_h
#define wukong_front_server_h

#include "corpc_queue.h"
#include "corpc_semaphore.h"
#include "corpc_rpc_client.h"

#include <string>
#include <vector>
#include <thread>

using namespace corpc;

namespace wukong {
    class TransportConnection: public std::enable_shared_from_this<TransportConnection> {
    public:
        TransportConnection(): clientFd(0), gateFd(0), closeSem(0) {}
        virtual ~TransportConnection() {}

        std::shared_ptr<TransportConnection> getPtr() {
            return shared_from_this();
        }

    public:
        int clientFd;
        int gateFd;

        Semaphore closeSem;
    };

    typedef MPMC_NoLockBlockQueue<std::shared_ptr<TransportConnection>> TransportConnectionQueue;

    // 单例模式实现
    class FrontServer {
    public:
        static FrontServer& Instance() {
            static FrontServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        static void inflowThreadEntry( );
        static void outflowThreadEntry( );

        static void *acceptRoutine( void * arg );
        static void *inflowQueueConsumeRoutine( void * arg );
        static void *outflowQueueConsumeRoutine( void * arg );
        static void *authRoutine( void * arg );
        static void *inflowRoutine( void * arg );
        static void *outflowRoutine( void * arg );

    private:
        bool inited_ = false;

        std::string ip_;
        uint16_t port_;

        sockaddr_in localAddr_;
        int listenFd_;

        IO *io_ = nullptr;
        RpcClient *rpcClient_ = nullptr;

        TransportConnectionQueue inflowQueue_;
        TransportConnectionQueue outflowQueue_;

        std::vector<std::thread> threads_;

    private:
        FrontServer() = default;                                  // ctor hidden
        FrontServer(FrontServer const&) = delete;                 // copy ctor hidden
        FrontServer(FrontServer &&) = delete;                     // move ctor hidden
        FrontServer& operator=(FrontServer const&) = delete;      // assign op. hidden
        FrontServer& operator=(FrontServer &&) = delete;          // move assign op. hidden
        ~FrontServer() = default;                                 // dtor hidden
    };

    #define g_FrontServer FrontServer::Instance()
}

#endif /* wukong_front_server_h */

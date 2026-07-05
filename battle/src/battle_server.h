/*
 * Created by Xianke Liu on 2025/12/26.
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

#ifndef wukong_battle_server_h
#define wukong_battle_server_h

#include "corpc_rpc_client.h"
#include "share/define.h"
#include "battle_service.pb.h"
#include <map>
#include <thread>

using namespace corpc;

namespace wukong {

    // 单例模式实现
    class BattleServer {
    public:
        static BattleServer& Instance() {
            static BattleServer theSingleton;
            return theSingleton;
        }

        bool init(int argc, char * argv[]);
        void run();

    private:
        bool inited_ = false;
        IO *io_ = nullptr;
        RpcClient *rpcClient_ = nullptr;
        
    private:
        BattleServer() = default;                                 // ctor hidden
        BattleServer(BattleServer const&) = delete;               // copy ctor hidden
        BattleServer(BattleServer &&) = delete;                   // move ctor hidden
        BattleServer& operator=(BattleServer const&) = delete;    // assign op. hidden
        BattleServer& operator=(BattleServer &&) = delete;        // move assign op. hidden
        ~BattleServer() = default;                                // dtor hidden
    };

}

#define g_BattleServer wukong::BattleServer::Instance()

#endif /* wukong_battle_server_h */

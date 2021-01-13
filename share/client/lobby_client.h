/*
 * Created by Xianke Liu on 2021/1/11.
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

#ifndef lobby_client_h
#define lobby_client_h

#include <map>
#include <vector>
#include "corpc_rpc_client.h"
#include "service_common.pb.h"
#include "game_service.pb.h"
#include "lobby_service.pb.h"
#include "define.h"

using namespace corpc;

namespace wukong {
    class LobbyClient {
    public:
        struct AddressInfo {
            uint16_t id;
            std::string ip;
            uint16_t port;
        };

        struct StubInfo {
            std::string ip; // rpc服务ip
            uint16_t port; // rpc服务port
            std::shared_ptr<pb::GameService_Stub> gameServiceStub;
            std::shared_ptr<pb::LobbyService_Stub> lobbyServiceStub;
        };

        struct ServerInfo {
            ServerId id;
            uint32_t count; // 在线人数
            uint32_t weight; // 分配权重(所有服务器的总在线人数 - 在线人数)
        };

    public:
        static LobbyClient& Instance() {
            static LobbyClient instance;
            return instance;
        }

        void init(RpcClient *client) { _client = client; }

        /* 业务逻辑 */
        std::vector<ServerInfo> getServerInfos(); // 注意：这里直接定义返回vector类型，通过编译器RVO优化
        void forward(ServerId sid, int32_t type, const std::vector<RoleId> &roleIds, const std::string &rawMsg);
    
        /* 加入Server */
        bool setServers(const std::map<ServerId, AddressInfo> &addresses);
        /* 根据逻辑区服id获得LobbyServer的stub */
        std::shared_ptr<pb::LobbyService_Stub> getLobbyServiceStub(ServerId sid);
        std::shared_ptr<pb::GameService_Stub> getGameServiceStub(ServerId sid);
        
        static bool parseAddress(const std::string &input, AddressInfo &addressInfo);

    private:
        void refreshStubs();

    private:
        RpcClient *_client = nullptr;

        /* 所有LobbyServer的Stub */
        static std::map<ServerId, StubInfo> _stubs;
        static std::mutex _stubsLock;
        static std::atomic<uint32_t> _stubChangeNum;
        
        /* 当前可用的 */
        static thread_local std::map<ServerId, StubInfo> _t_stubs;
        static thread_local uint32_t _t_stubChangeNum;

    private:
        LobbyClient() = default;                                // ctor hidden
        ~LobbyClient() = default;                               // destruct hidden
        LobbyClient(LobbyClient const&) = delete;               // copy ctor delete
        LobbyClient(LobbyClient &&) = delete;                   // move ctor delete
        LobbyClient& operator=(LobbyClient const&) = delete;    // assign op. delete
        LobbyClient& operator=(LobbyClient &&) = delete;        // move assign op. delete
    };
}
    
#define g_LobbyClient wukong::LobbyClient::Instance()

#endif /* lobby_client_h */

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
#include "game_service.pb.h"
#include "lobby_service.pb.h"
#include "game_client.h"
#include "define.h"
#include "const.h"

using namespace corpc;

namespace wukong {
    class LobbyClient: public GameClient {
    public:
        struct StubInfo {
            std::string rpcAddr; // rpc服务地址"ip:port"
            std::shared_ptr<pb::GameService_Stub> gameServiceStub;
            std::shared_ptr<pb::LobbyService_Stub> lobbyServiceStub;
        };

    public:
        static LobbyClient& Instance() {
            static LobbyClient instance;
            return instance;
        }

        virtual std::vector<ServerInfo> getServerInfos();
        virtual bool setServers(const std::vector<AddressInfo> &addresses);
        virtual void forwardIn(ServerId sid, int16_t type, uint16_t tag, RoleId roleId, const std::string &rawMsg);

        virtual std::shared_ptr<pb::GameService_Stub> getGameServiceStub(ServerId sid);
        
        std::shared_ptr<pb::LobbyService_Stub> getLobbyServiceStub(ServerId sid);
        
        bool loadRole(ServerId sid, RoleId roleId, ServerId gwId); // 加载角色（游戏对象）
        void shutdown();

        bool stubChanged() { return _stubChangeNum != _t_stubChangeNum; }
        
    private:
        void refreshStubs();

    private:
        /* 所有LobbyServer的Stub */
        static std::map<std::string, std::pair<std::shared_ptr<pb::GameService_Stub>, std::shared_ptr<pb::LobbyService_Stub>>> _addr2stubs; // 用于保持被_stubs中的StubInfo引用（不直接访问）
        static std::map<ServerId, StubInfo> _stubs;
        static std::mutex _stubsLock;
        static std::atomic<uint32_t> _stubChangeNum;
        
        /* 当前可用的 */
        static thread_local std::map<ServerId, StubInfo> _t_stubs;
        static thread_local uint32_t _t_stubChangeNum;

    private:
        LobbyClient(): GameClient(GAME_SERVER_TYPE_LOBBY, ZK_LOBBY_SERVER) {} // ctor hidden
        virtual ~LobbyClient() = default;                       // destruct hidden
        LobbyClient(LobbyClient const&) = delete;               // copy ctor delete
        LobbyClient(LobbyClient &&) = delete;                   // move ctor delete
        LobbyClient& operator=(LobbyClient const&) = delete;    // assign op. delete
        LobbyClient& operator=(LobbyClient &&) = delete;        // move assign op. delete
    };
}
    
#define g_LobbyClient wukong::LobbyClient::Instance()

#endif /* lobby_client_h */

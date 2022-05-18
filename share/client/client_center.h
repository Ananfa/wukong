/*
 * Created by Xianke Liu on 2022/2/17.
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

#ifndef wukong_client_center_h
#define wukong_client_center_h

#include "corpc_redis.h"
#include "gateway_client.h"
#include "record_client.h"
#include "lobby_client.h"
#include "scene_client.h"
#include "share/define.h"

#include <vector>
#include <mutex>
#include <atomic>

using namespace corpc;

namespace wukong {
    class ClientCenter
    {
    public:
        static ClientCenter& Instance() {
            static ClientCenter instance;
            return instance;
        }

        void init(RpcClient *rpcc, const std::string& zooAddr, const std::string& zooPath, bool connectGateway, bool connectRecord, bool connectLobby, bool connectScene);
        
        bool randomGatewayServer(ServerId &serverId);
        bool randomRecordServer(ServerId &serverId);
        bool randomLobbyServer(ServerId &serverId);
        bool randomSceneServer(uint32_t type, ServerId &serverId);

        Address getGatewayAddress(ServerId gatewayId) { return _t_gatewayAddrMap[gatewayId]; }

    private:
        void updateGatewayInfosVersion() { _gatewayInfosVersion++; };
        void updateRecordInfosVersion() { _recordInfosVersion++; };
        void updateLobbyInfosVersion() { _lobbyInfosVersion++; };
        void updateSceneInfosVersion() { _sceneInfosVersion++; };

        // 利用"所有服务器的总在线人数 - 在线人数"做为分配权重
        void refreshGatewayInfos();
        void refreshRecordInfos();
        void refreshLobbyInfos();
        void refreshSceneInfos();

        static void *updateRoutine(void *arg);
        void updateGatewayInfos();
        void updateRecordInfos();
        void updateLobbyInfos();
        void updateSceneInfos();

    private:
        // gateway服信息
        static std::vector<GatewayClient::ServerInfo> _gatewayInfos;
        static std::mutex _gatewayInfosLock;
        static std::atomic<uint32_t> _gatewayInfosVersion;

        static thread_local std::vector<ServerWeightInfo> _t_gatewayInfos;
        static thread_local std::map<ServerId, Address> _t_gatewayAddrMap;
        static thread_local uint32_t _t_gatewayInfosVersion;
        static thread_local uint32_t _t_gatewayTotalWeight;

        // record服信息
        static std::vector<RecordClient::ServerInfo> _recordInfos;
        static std::mutex _recordInfosLock;
        static std::atomic<uint32_t> _recordInfosVersion;

        static thread_local std::vector<ServerWeightInfo> _t_recordInfos;
        static thread_local uint32_t _t_recordInfosVersion;
        static thread_local uint32_t _t_recordTotalWeight;

        // lobby服信息
        static std::vector<LobbyClient::ServerInfo> _lobbyInfos;
        static std::mutex _lobbyInfosLock;
        static std::atomic<uint32_t> _lobbyInfosVersion;

        static thread_local std::vector<ServerWeightInfo> _t_lobbyInfos;
        static thread_local uint32_t _t_lobbyInfosVersion;
        static thread_local uint32_t _t_lobbyTotalWeight;

        // scene服信息
        static std::vector<SceneClient::ServerInfo> _sceneInfos; // TODO: 场景服应该根据类型来分类
        static std::mutex _sceneInfosLock;
        static std::atomic<uint32_t> _sceneInfosVersion;

        static thread_local std::map<uint32_t, std::vector<ServerWeightInfo>> _t_sceneInfos;
        static thread_local uint32_t _t_sceneInfosVersion;
        static thread_local std::map<uint32_t, uint32_t> _t_sceneTotalWeights;

    private:
        ClientCenter() = default;                                      // ctor hidden
        ~ClientCenter() = default;                                     // destruct hidden
        ClientCenter(ClientCenter const&) = delete;                    // copy ctor delete
        ClientCenter(ClientCenter &&) = delete;                        // move ctor delete
        ClientCenter& operator=(ClientCenter const&) = delete;         // assign op. delete
        ClientCenter& operator=(ClientCenter &&) = delete;             // move assign op. delete
    };
}

#define g_ClientCenter ClientCenter::Instance()

#endif /* wukong_client_center_h */

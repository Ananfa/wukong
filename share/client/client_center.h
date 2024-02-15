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
#include "corpc_mutex.h"
#include "gateway_client.h"
#include "record_client.h"
#include "lobby_client.h"
#include "scene_client.h"
#include "share/define.h"

#include <vector>
//#include <mutex>
#include <atomic>

using namespace corpc;

namespace wukong {
    class ClientCenter {
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

        Address getGatewayAddress(ServerId gatewayId) { return t_gatewayAddrMap_[gatewayId]; }

    private:
        void updateGatewayInfosVersion() { gatewayInfosVersion_++; };
        void updateRecordInfosVersion() { recordInfosVersion_++; };
        void updateLobbyInfosVersion() { lobbyInfosVersion_++; };
        void updateSceneInfosVersion() { sceneInfosVersion_++; };

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
        static std::vector<GatewayClient::ServerInfo> gatewayInfos_;
        static Mutex gatewayInfosLock_;
        static std::atomic<uint32_t> gatewayInfosVersion_;

        static thread_local std::vector<ServerWeightInfo> t_gatewayInfos_;
        static thread_local std::map<ServerId, Address> t_gatewayAddrMap_;
        static thread_local uint32_t t_gatewayInfosVersion_;
        static thread_local uint32_t t_gatewayTotalWeight_;

        // record服信息
        static std::vector<RecordClient::ServerInfo> recordInfos_;
        static Mutex recordInfosLock_;
        static std::atomic<uint32_t> recordInfosVersion_;

        static thread_local std::vector<ServerWeightInfo> t_recordInfos_;
        static thread_local uint32_t t_recordInfosVersion_;
        static thread_local uint32_t t_recordTotalWeight_;

        // lobby服信息
        static std::vector<LobbyClient::ServerInfo> lobbyInfos_;
        static Mutex lobbyInfosLock_;
        static std::atomic<uint32_t> lobbyInfosVersion_;

        static thread_local std::vector<ServerWeightInfo> t_lobbyInfos_;
        static thread_local uint32_t t_lobbyInfosVersion_;
        static thread_local uint32_t t_lobbyTotalWeight_;

        // scene服信息
        static std::vector<SceneClient::ServerInfo> sceneInfos_; // TODO: 场景服应该根据类型来分类
        static Mutex sceneInfosLock_;
        static std::atomic<uint32_t> sceneInfosVersion_;

        static thread_local std::map<uint32_t, std::vector<ServerWeightInfo>> t_sceneInfos_;
        static thread_local uint32_t t_sceneInfosVersion_;
        static thread_local std::map<uint32_t, uint32_t> t_sceneTotalWeights_;

    private:
        ClientCenter() = default;                                      // ctor hidden
        ~ClientCenter() = default;                                     // destruct hidden
        ClientCenter(ClientCenter const&) = delete;                    // copy ctor delete
        ClientCenter(ClientCenter &&) = delete;                        // move ctor delete
        ClientCenter& operator=(ClientCenter const&) = delete;         // assign op. delete
        ClientCenter& operator=(ClientCenter &&) = delete;             // move assign op. delete
    };
}

#define g_ClientCenter wukong::ClientCenter::Instance()

#endif /* wukong_client_center_h */

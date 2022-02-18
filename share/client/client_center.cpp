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

#include "client_center.h"
#include "share/const.h"

using namespace wukong;

std::vector<GatewayClient::ServerInfo> ClientCenter::_gatewayInfos;
std::mutex ClientCenter::_gatewayInfosLock;
std::atomic<uint32_t> ClientCenter::_gatewayInfosVersion(0);
thread_local std::vector<ServerWeightInfo> ClientCenter::_t_gatewayInfos;
thread_local std::map<ServerId, Address> ClientCenter::_t_gatewayAddrMap;
thread_local uint32_t ClientCenter::_t_gatewayInfosVersion(0);
thread_local uint32_t ClientCenter::_t_gatewayTotalWeight(0);

std::vector<RecordClient::ServerInfo> ClientCenter::_recordInfos;
std::mutex ClientCenter::_recordInfosLock;
std::atomic<uint32_t> ClientCenter::_recordInfosVersion(0);
thread_local std::vector<ServerWeightInfo> ClientCenter::_t_recordInfos;
thread_local uint32_t ClientCenter::_t_recordInfosVersion(0);
thread_local uint32_t ClientCenter::_t_recordTotalWeight(0);

std::vector<LobbyClient::ServerInfo> ClientCenter::_lobbyInfos;
std::mutex ClientCenter::_lobbyInfosLock;
std::atomic<uint32_t> ClientCenter::_lobbyInfosVersion(0);
thread_local std::vector<ServerWeightInfo> ClientCenter::_t_lobbyInfos;
thread_local uint32_t ClientCenter::_t_lobbyInfosVersion(0);
thread_local uint32_t ClientCenter::_t_lobbyTotalWeight(0);

std::vector<SceneClient::ServerInfo> ClientCenter::_sceneInfos;
std::mutex ClientCenter::_sceneInfosLock;
std::atomic<uint32_t> ClientCenter::_sceneInfosVersion(0);
thread_local std::vector<ServerWeightInfo> ClientCenter::_t_sceneInfos;
thread_local uint32_t ClientCenter::_t_sceneInfosVersion(0);
thread_local uint32_t ClientCenter::_t_sceneTotalWeight(0);

void ClientCenter::init() {
    RoutineEnvironment::startCoroutine(updateRoutine, this);
}

void *ClientCenter::updateRoutine(void *arg) {
    ClientCenter *self = (ClientCenter *)arg;

    // 每秒检查是否有record的增加或减少，如果有马上刷新，否则每分钟刷新一次record的负载信息
    int i = 0;
    while (true) {
        i++;
        if (i >= 60) {
            self->updateGatewayInfos();
            self->updateRecordInfos();
            self->updateLobbyInfos();
            self->updateSceneInfos();
            i = 0;
        } else {
            if (g_GatewayClient.stubChanged()) {
                self->updateGatewayInfos();
            }

            if (g_RecordClient.stubChanged()) {
                self->updateRecordInfos();
            }

            if (g_LobbyClient.stubChanged()) {
                self->updateLobbyInfos();
            }

            if (g_SceneClient.stubChanged()) {
                self->updateSceneInfos();
            }
        }

        sleep(1);
    }
    
    return nullptr;
}

void ClientCenter::updateGatewayInfos() {
    std::vector<GatewayClient::ServerInfo> infos = g_GatewayClient.getServerInfos();
    {
        std::unique_lock<std::mutex> lock(_gatewayInfosLock);
        _gatewayInfos = std::move(infos);
        updateGatewayInfosVersion();
    }
    DEBUG_LOG("update gateway server info\n");
}

void ClientCenter::updateRecordInfos() {
    std::vector<RecordClient::ServerInfo> infos = g_RecordClient.getServerInfos();
    {
        std::unique_lock<std::mutex> lock(_recordInfosLock);
        _recordInfos = std::move(infos);
        updateRecordInfosVersion();
    }
    DEBUG_LOG("update record server info\n");
}

void ClientCenter::updateLobbyInfos() {
    std::vector<LobbyClient::ServerInfo> infos = g_LobbyClient.getServerInfos();
    {
        std::unique_lock<std::mutex> lock(_lobbyInfosLock);
        _lobbyInfos = std::move(infos);
        updateLobbyInfosVersion();
    }
    DEBUG_LOG("update lobby server info\n");
}

void ClientCenter::updateSceneInfos() {
    std::vector<SceneClient::ServerInfo> infos = g_SceneClient.getServerInfos();
    {
        std::unique_lock<std::mutex> lock(_sceneInfosLock);
        _sceneInfos = std::move(infos);
        updateSceneInfosVersion();
    }
    DEBUG_LOG("update scene server info\n");
}

bool ClientCenter::randomGatewayServer(ServerId &serverId) {
    refreshGatewayInfos();
    size_t serverNum = _t_gatewayInfos.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = _t_gatewayTotalWeight;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += _t_gatewayInfos[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = _t_gatewayInfos[i].id;

    // 调整权重
    _t_gatewayTotalWeight += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        _t_gatewayInfos[j].weight++;
    }
    _t_gatewayInfos[i].weight--;
    return true;
}

bool ClientCenter::randomRecordServer(ServerId &serverId) {
    refreshRecordInfos();
    size_t serverNum = _t_recordInfos.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = _t_recordTotalWeight;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += _t_recordInfos[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = _t_recordInfos[i].id;

    // 调整权重
    _t_recordTotalWeight += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        _t_recordInfos[j].weight++;
    }
    _t_recordInfos[i].weight--;
    return true;
}

bool ClientCenter::randomLobbyServer(ServerId &serverId) {
    refreshLobbyInfos();
    size_t serverNum = _t_lobbyInfos.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = _t_lobbyTotalWeight;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += _t_lobbyInfos[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = _t_lobbyInfos[i].id;

    // 调整权重
    _t_lobbyTotalWeight += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        _t_lobbyInfos[j].weight++;
    }
    _t_lobbyInfos[i].weight--;
    return true;
}

bool ClientCenter::randomSceneServer(uint32_t type, ServerId &serverId) {
    refreshSceneInfos();
    size_t serverNum = _t_sceneInfos.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = _t_sceneTotalWeights[type];
    auto &sceneInfos = _t_sceneInfos[type];

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += sceneInfos[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = sceneInfos[i].id;

    // 调整权重
    _t_sceneTotalWeights[type] += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        sceneInfos[j].weight++;
    }
    sceneInfos[i].weight--;
    return true;
}

void ClientCenter::refreshGatewayInfos() {
    if (_t_gatewayInfosVersion != _gatewayInfosVersion) {
        LOG("refresh gateway info\n");
        _t_gatewayInfos.clear();
        _t_gatewayAddrMap.clear();

        std::vector<GatewayClient::ServerInfo> gatewayInfos;

        {
            std::unique_lock<std::mutex> lock(_gatewayInfosLock);
            gatewayInfos = _gatewayInfos;
            _t_gatewayInfosVersion = _gatewayInfosVersion;
        }

        _t_gatewayInfos.reserve(gatewayInfos.size());
        uint32_t totalWeight = 0;
        for (auto &info : gatewayInfos) {
            DEBUG_LOG("gateway id:%d\n", info.id);
            totalWeight += info.weight;

            _t_gatewayInfos.push_back({info.id, info.weight});

            Address gatewayAddr = {info.outerAddr, info.outerPort};
            _t_gatewayAddrMap.insert(std::make_pair(info.id, std::move(gatewayAddr)));
        }
        
        _t_gatewayTotalWeight = totalWeight;
    }
}

void ClientCenter::refreshRecordInfos() {
    if (_t_recordInfosVersion != _recordInfosVersion) {
        _t_recordInfos.clear();

        std::vector<RecordClient::ServerInfo> recordInfos;

        {
            std::unique_lock<std::mutex> lock(_recordInfosLock);
            recordInfos = _recordInfos;
            _t_recordInfosVersion = _recordInfosVersion;
        }

        _t_recordInfos.reserve(recordInfos.size());
        uint32_t totalWeight = 0;
        for (auto &info : recordInfos) {
            totalWeight += info.weight;

            _t_recordInfos.push_back({info.id, info.weight});
        }
        
        _t_recordTotalWeight = totalWeight;
    }
}

void ClientCenter::refreshLobbyInfos() {
    if (_t_lobbyInfosVersion != _lobbyInfosVersion) {
        _t_lobbyInfos.clear();

        std::vector<GameClient::ServerInfo> lobbyInfos;

        {
            std::unique_lock<std::mutex> lock(_lobbyInfosLock);
            lobbyInfos = _lobbyInfos;
            _t_lobbyInfosVersion = _lobbyInfosVersion;
        }

        _t_lobbyInfos.reserve(lobbyInfos.size());
        uint32_t totalWeight = 0;
        for (auto &info : lobbyInfos) {
            totalWeight += info.weight;

            _t_lobbyInfos.push_back({info.id, info.weight});
        }
        
        _t_lobbyTotalWeight = totalWeight;
    }
}

void ClientCenter::refreshSceneInfos() {
    if (_t_sceneInfosVersion != _sceneInfosVersion) {
        _t_sceneInfos.clear();

        std::vector<GameClient::ServerInfo> sceneInfos;

        {
            std::unique_lock<std::mutex> lock(_sceneInfosLock);
            sceneInfos = _sceneInfos;
            _t_sceneInfosVersion = _sceneInfosVersion;
        }

        std::map<uint32_t, uint32_t> totalWeights
        for (auto &info : sceneInfos) {
            totalWeights[info.type] += info.weight;

            _t_sceneInfos[info.type].push_back({info.id, info.weight});
        }

        _t_sceneTotalWeights = std::move(totalWeights);
    }
}



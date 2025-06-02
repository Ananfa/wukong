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
#if 0
#include "client_center.h"
#include "share/const.h"

#include "zk_client.h"

using namespace corpc;
using namespace wukong;

std::vector<GatewayClient::ServerInfo> ClientCenter::gatewayInfos_;
Mutex ClientCenter::gatewayInfosLock_;
std::atomic<uint32_t> ClientCenter::gatewayInfosVersion_(0);
thread_local std::vector<ServerWeightInfo> ClientCenter::t_gatewayInfos_;
thread_local std::map<ServerId, Address> ClientCenter::t_gatewayAddrMap_;
thread_local uint32_t ClientCenter::t_gatewayInfosVersion_(0);
thread_local uint32_t ClientCenter::t_gatewayTotalWeight_(0);

std::vector<RecordClient::ServerInfo> ClientCenter::recordInfos_;
Mutex ClientCenter::recordInfosLock_;
std::atomic<uint32_t> ClientCenter::recordInfosVersion_(0);
thread_local std::vector<ServerWeightInfo> ClientCenter::t_recordInfos_;
thread_local uint32_t ClientCenter::t_recordInfosVersion_(0);
thread_local uint32_t ClientCenter::t_recordTotalWeight_(0);

std::vector<LobbyClient::ServerInfo> ClientCenter::lobbyInfos_;
Mutex ClientCenter::lobbyInfosLock_;
std::atomic<uint32_t> ClientCenter::lobbyInfosVersion_(0);
thread_local std::vector<ServerWeightInfo> ClientCenter::t_lobbyInfos_;
thread_local uint32_t ClientCenter::t_lobbyInfosVersion_(0);
thread_local uint32_t ClientCenter::t_lobbyTotalWeight_(0);

std::vector<SceneClient::ServerInfo> ClientCenter::sceneInfos_;
Mutex ClientCenter::sceneInfosLock_;
std::atomic<uint32_t> ClientCenter::sceneInfosVersion_(0);
thread_local std::map<uint32_t, std::vector<ServerWeightInfo>> ClientCenter::t_sceneInfos_;
thread_local uint32_t ClientCenter::t_sceneInfosVersion_(0);
thread_local std::map<uint32_t, uint32_t> ClientCenter::t_sceneTotalWeights_;

void ClientCenter::init(RpcClient *rpcc, const std::string& zooAddr, const std::string& zooPath, bool connectGateway, bool connectRecord, bool connectLobby, bool connectScene) {
    if (connectGateway) {
        g_GatewayClient.init(rpcc);
    }

    if (connectRecord) {
        g_RecordClient.init(rpcc);
    }

    if (connectLobby) {
        g_LobbyClient.init(rpcc);
    }

    if (connectScene) {
        g_SceneClient.init(rpcc);
    }

    // zoo注册
    g_ZkClient.init(zooAddr, ZK_TIMEOUT, [zooPath,connectGateway,connectRecord,connectLobby,connectScene]() { // 注意: 此处lamda的capture参数不能用引用方式，因为lamda会在另外的协程运行，此时丢失了init方法所在协程的栈上下文
        // 对servers配置中每一个server进行节点注册
        g_ZkClient.createEphemeralNode(zooPath, ZK_DEFAULT_VALUE, [](const std::string &path, const ZkRet &ret) {
            if (ret) {
                LOG("create rpc node:[%s] sucessful\n", path.c_str());
            } else {
                ERROR_LOG("create rpc node:[%d] failed, code = %d\n", path.c_str(), ret.code());
            }
        });

        if (connectGateway) {
            g_ZkClient.watchChildren(ZK_GATEWAY_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
                std::vector<GatewayClient::AddressInfo> addresses;
                addresses.reserve(values.size());
                for (const std::string &value : values) {
                    GatewayClient::AddressInfo address;

                    if (GatewayClient::parseAddress(value, address)) {
                        addresses.push_back(std::move(address));
                    } else {
                        ERROR_LOG("zkclient parse gateway server address error, info = %s\n", value.c_str());
                    }
                }
                g_GatewayClient.setServers(addresses);
            });
        }

        if (connectRecord) {
            g_ZkClient.watchChildren(ZK_RECORD_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
                std::vector<RecordClient::AddressInfo> addresses;
                addresses.reserve(values.size());
                for (const std::string &value : values) {
                    RecordClient::AddressInfo address;

                    if (RecordClient::parseAddress(value, address)) {
                        addresses.push_back(std::move(address));
                    } else {
                        ERROR_LOG("zkclient parse record server address error, info = %s\n", value.c_str());
                    }
                }
                g_RecordClient.setServers(addresses);
            });
        }

        if (connectLobby) {
            g_ZkClient.watchChildren(ZK_LOBBY_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
                std::vector<GameClient::AddressInfo> addresses;
                addresses.reserve(values.size());
                for (const std::string &value : values) {
                    GameClient::AddressInfo address;

                    if (GameClient::parseAddress(value, address)) {
                        addresses.push_back(std::move(address));
                    } else {
                        ERROR_LOG("zkclient parse lobby server address error, info = %s\n", value.c_str());
                    }
                }
                g_LobbyClient.setServers(addresses);
            });
        }

        if (connectScene) {
            g_ZkClient.watchChildren(ZK_SCENE_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
                std::vector<SceneClient::AddressInfo> addresses;
                addresses.reserve(values.size());
                for (const std::string &value : values) {
                    SceneClient::AddressInfo address;

                    if (GameClient::parseAddress(value, address)) {
                        addresses.push_back(std::move(address));
                    } else {
                        ERROR_LOG("zkclient parse scene server address error, info = %s\n", value.c_str());
                    }
                }
                g_SceneClient.setServers(addresses);
            });
        }
    });

    RoutineEnvironment::startCoroutine(updateRoutine, this);
}

void *ClientCenter::updateRoutine(void *arg) {
    ClientCenter *self = (ClientCenter *)arg;

    // 每秒检查是否有服务的增加或减少，如果有马上刷新，否则每分钟刷新一次服务的负载信息
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
        LockGuard lock(gatewayInfosLock_);
        gatewayInfos_ = std::move(infos);
        updateGatewayInfosVersion();
    }
    DEBUG_LOG("update gateway server info\n");
}

void ClientCenter::updateRecordInfos() {
    std::vector<RecordClient::ServerInfo> infos = g_RecordClient.getServerInfos();
    {
        LockGuard lock(recordInfosLock_);
        recordInfos_ = std::move(infos);
        updateRecordInfosVersion();
    }
    DEBUG_LOG("update record server info\n");
}

void ClientCenter::updateLobbyInfos() {
    std::vector<LobbyClient::ServerInfo> infos = g_LobbyClient.getServerInfos();
    {
        LockGuard lock(lobbyInfosLock_);
        lobbyInfos_ = std::move(infos);
        updateLobbyInfosVersion();
    }
    DEBUG_LOG("update lobby server info\n");
}

void ClientCenter::updateSceneInfos() {
    std::vector<SceneClient::ServerInfo> infos = g_SceneClient.getServerInfos();
    {
        LockGuard lock(sceneInfosLock_);
        sceneInfos_ = std::move(infos);
        updateSceneInfosVersion();
    }
    DEBUG_LOG("update scene server info\n");
}

bool ClientCenter::randomGatewayServer(ServerId &serverId) {
    refreshGatewayInfos();
    size_t serverNum = t_gatewayInfos_.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = t_gatewayTotalWeight_;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += t_gatewayInfos_[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = t_gatewayInfos_[i].id;

    // 调整权重
    t_gatewayTotalWeight_ += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        t_gatewayInfos_[j].weight++;
    }
    t_gatewayInfos_[i].weight--;
    return true;
}

bool ClientCenter::randomRecordServer(ServerId &serverId) {
    refreshRecordInfos();
    size_t serverNum = t_recordInfos_.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = t_recordTotalWeight_;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += t_recordInfos_[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = t_recordInfos_[i].id;

    // 调整权重
    t_recordTotalWeight_ += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        t_recordInfos_[j].weight++;
    }
    t_recordInfos_[i].weight--;
    return true;
}

bool ClientCenter::randomLobbyServer(ServerId &serverId) {
    refreshLobbyInfos();
    size_t serverNum = t_lobbyInfos_.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = t_lobbyTotalWeight_;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += t_lobbyInfos_[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = t_lobbyInfos_[i].id;

    // 调整权重
    t_lobbyTotalWeight_ += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        t_lobbyInfos_[j].weight++;
    }
    t_lobbyInfos_[i].weight--;
    return true;
}

bool ClientCenter::randomSceneServer(uint32_t type, ServerId &serverId) {
    refreshSceneInfos();
    size_t serverNum = t_sceneInfos_.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = t_sceneTotalWeights_[type];
    auto &sceneInfos = t_sceneInfos_[type];

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
    t_sceneTotalWeights_[type] += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        sceneInfos[j].weight++;
    }
    sceneInfos[i].weight--;
    return true;
}

void ClientCenter::refreshGatewayInfos() {
    if (t_gatewayInfosVersion_ != gatewayInfosVersion_) {
        LOG("refresh gateway info\n");
        //std::vector<GatewayClient::ServerInfo> gatewayInfos;
        {
            LockGuard lock(gatewayInfosLock_);

            if (t_gatewayInfosVersion_ == gatewayInfosVersion_) {
                return;
            }

            //gatewayInfos = gatewayInfos_;
            t_gatewayInfosVersion_ = gatewayInfosVersion_;

            t_gatewayInfos_.clear();
            t_gatewayAddrMap_.clear();
            t_gatewayInfos_.reserve(gatewayInfos_.size());
            uint32_t totalWeight = 0;
            for (auto &info : gatewayInfos_) {
                //DEBUG_LOG("gateway id:%d\n", info.id);
                totalWeight += info.weight;

                t_gatewayInfos_.push_back({info.id, info.weight});

                Address gatewayAddr = {info.outerAddr, info.outerPort};
                t_gatewayAddrMap_.insert(std::make_pair(info.id, std::move(gatewayAddr)));
            }
            
            t_gatewayTotalWeight_ = totalWeight;
        }
    }
}

void ClientCenter::refreshRecordInfos() {
    if (t_recordInfosVersion_ != recordInfosVersion_) {
        //std::vector<RecordClient::ServerInfo> recordInfos;
        {
            LockGuard lock(recordInfosLock_);

            if (t_recordInfosVersion_ == recordInfosVersion_) {
                return;
            }

            //recordInfos = recordInfos_;
            t_recordInfosVersion_ = recordInfosVersion_;

            t_recordInfos_.clear();
            t_recordInfos_.reserve(recordInfos_.size());
            uint32_t totalWeight = 0;
            for (auto &info : recordInfos_) {
                totalWeight += info.weight;

                t_recordInfos_.push_back({info.id, info.weight});
            }
            
            t_recordTotalWeight_ = totalWeight;
        }
    }
}

void ClientCenter::refreshLobbyInfos() {
    if (t_lobbyInfosVersion_ != lobbyInfosVersion_) {
        //std::vector<GameClient::ServerInfo> lobbyInfos;
        {
            LockGuard lock(lobbyInfosLock_);

            if (t_lobbyInfosVersion_ == lobbyInfosVersion_) {
                return;
            }

            //lobbyInfos = lobbyInfos_;
            t_lobbyInfosVersion_ = lobbyInfosVersion_;

            t_lobbyInfos_.clear();
            t_lobbyInfos_.reserve(lobbyInfos_.size());
            uint32_t totalWeight = 0;
            for (auto &info : lobbyInfos_) {
                totalWeight += info.weight;

                t_lobbyInfos_.push_back({info.id, info.weight});
            }
            
            t_lobbyTotalWeight_ = totalWeight;
        }
    }
}

void ClientCenter::refreshSceneInfos() {
    if (t_sceneInfosVersion_ != sceneInfosVersion_) {
        //std::vector<GameClient::ServerInfo> sceneInfos;
        {
            LockGuard lock(sceneInfosLock_);

            if (t_sceneInfosVersion_ == sceneInfosVersion_) {
                return;
            }

            //sceneInfos = sceneInfos_;
            t_sceneInfosVersion_ = sceneInfosVersion_;

            t_sceneInfos_.clear();
            std::map<uint32_t, uint32_t> totalWeights;
            for (auto &info : sceneInfos_) {
                totalWeights[info.type] += info.weight;

                t_sceneInfos_[info.type].push_back({info.id, info.weight});
            }

            t_sceneTotalWeights_ = std::move(totalWeights);
        }
    }
}

#endif

/*
 * Created by Xianke Liu on 2022/2/16.
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
#include "scene_client.h"
#include "corpc_controller.h"

using namespace corpc;
using namespace wukong;

std::map<std::string, std::pair<std::shared_ptr<pb::GameService_Stub>, std::shared_ptr<pb::SceneService_Stub>>> SceneClient::addr2stubs_;
std::map<ServerId, SceneClient::StubInfo> SceneClient::stubs_;
Mutex SceneClient::stubsLock_;
std::atomic<uint32_t> SceneClient::stubChangeNum_(0);
thread_local uint32_t SceneClient::t_stubChangeNum_ = 0;
thread_local std::map<ServerId, SceneClient::StubInfo> SceneClient::t_stubs_;

void SceneClient::shutdown() {
    refreshStubs();
    std::map<ServerId, SceneClient::StubInfo> stubs = t_stubs_;
    
    for (const auto& kv : stubs) {
        corpc::Void *request = new corpc::Void();
        
        kv.second.sceneServiceStub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request));
    }
}

std::vector<GameClient::ServerInfo> SceneClient::getServerInfos() {
    std::vector<ServerInfo> infos;

    // 对相同rpc地址的服务不需要重复获取
    std::map<std::string, bool> handledAddrs; // 记录已处理的rpc地址

    refreshStubs();
    std::map<ServerId, StubInfo> localStubs = t_stubs_;
    
    // 清除原来的信息
    infos.reserve(localStubs.size());

    // 利用(总在线人数 - 服务器在线人数)作为分配服务器的权重
    std::map<uint32_t, uint32_t> totalCounts;
    for (auto &kv : localStubs) {
        if (handledAddrs.find(kv.second.rpcAddr) != handledAddrs.end()) {
            continue;
        }

        handledAddrs.insert(std::make_pair(kv.second.rpcAddr, true));

        corpc::Void *request = new corpc::Void();
        pb::OnlineCounts *response = new pb::OnlineCounts();
        Controller *controller = new Controller();
        kv.second.sceneServiceStub->getOnlineCount(controller, request, response, nullptr);
        
        if (controller->Failed()) {
            ERROR_LOG("SceneClient::getServerInfos -- Rpc Call Failed : %s\n", controller->ErrorText().c_str());
            continue;
        } else {
            auto countInfos = response->counts();

            for (auto it = countInfos.begin(); it != countInfos.end(); ++it) {
                infos.push_back({it->serverid(), kv.second.type, it->count(), 0});
                totalCounts[kv.second.type] += it->count();
            }
        }
        
        delete controller;
        delete response;
        delete request;
    }
    
    for (auto &info : infos) {
        info.weight = totalCounts[info.type] - info.count;
    }

    return infos;
}

std::string SceneClient::loadScene(ServerId sid, uint32_t defId, const std::string &sceneId, RoleId roleId, const std::string &teamId) {
    ERROR_LOG("SceneClient::loadScene : %s\n", sceneId.c_str());
    std::shared_ptr<pb::SceneService_Stub> stub = getSceneServiceStub(sid);
    std::string ret;

    if (!stub) {
        ERROR_LOG("SceneClient::loadScene -- scene server %d stub not avaliable, waiting.\n", sid);
        return ret;
    }

    pb::LoadSceneRequest *request = new pb::LoadSceneRequest();
    pb::LoadSceneResponse *response = new pb::LoadSceneResponse();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_defid(defId);
    request->set_sceneid(sceneId);
    request->set_roleid(roleId);
    request->set_teamid(teamId);
    stub->loadScene(controller, request, response, nullptr);

    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else {
        if (response->errcode()) {
            ERROR_LOG("SceneClient::loadScene -- rpc return errCode:%d.\n", response->errcode());
        }

        ret = response->sceneid();
    }

    delete controller;
    delete response;
    delete request;

    return ret;
}

void SceneClient::enterScene(ServerId sid, const std::string &sceneId, RoleId roleId, ServerId gwId) {
    std::shared_ptr<pb::SceneService_Stub> stub = getSceneServiceStub(sid);

    if (!stub) {
        ERROR_LOG("SceneClient::enterScene -- scene server %d stub not avaliable, waiting.\n", sid);
        return;
    }

    pb::EnterSceneRequest *request = new pb::EnterSceneRequest();
    request->set_serverid(sid);
    request->set_roleid(roleId);
    request->set_gatewayid(gwId);
    request->set_sceneid(sceneId);
    stub->enterScene(nullptr, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request));
}

void SceneClient::forwardIn(ServerId sid, int16_t type, uint16_t tag, RoleId roleId, const std::string &msg) {
    std::shared_ptr<pb::GameService_Stub> stub = getGameServiceStub(sid);
    
    if (!stub) {
        ERROR_LOG("SceneClient::forwardIn -- scene server %d stub not avaliable, waiting.\n", sid);
        return;
    }
    
    pb::ForwardInRequest *request = new pb::ForwardInRequest();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_type(type);

    if (tag != 0) {
        request->set_tag(tag);
    }

    request->set_roleid(roleId);
    
    if (!msg.empty()) {
        request->set_rawmsg(msg);
    }
    
    stub->forwardIn(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request, controller));
}

bool SceneClient::setServers(const std::vector<AddressInfo> &addresses) {
    if (!client_) {
        ERROR_LOG("SceneClient::setServers -- not init\n");
        return false;
    }

    std::map<std::string, std::pair<std::shared_ptr<pb::GameService_Stub>, std::shared_ptr<pb::SceneService_Stub>>> addr2stubs;
    std::map<ServerId, StubInfo> stubs;
    for (const auto& address : addresses) {
        std::pair<std::shared_ptr<pb::GameService_Stub>, std::shared_ptr<pb::SceneService_Stub>> stubPair;
        std::string addr = address.ip + std::to_string(address.port);
        auto iter = addr2stubs.find(addr);
        if (iter != addr2stubs.end()) {
            ERROR_LOG("SceneClient::setServers -- duplicate rpc addr: %s\n", addr.c_str());
            continue;
        }

        iter = addr2stubs_.find(addr);
        if (iter != addr2stubs_.end()) {
            stubPair = iter->second;
        } else {
            RpcClient::Channel *channel = new RpcClient::Channel(client_, address.ip, address.port, 1);

            stubPair = std::make_pair(
                std::make_shared<pb::GameService_Stub>(channel, pb::GameService::STUB_OWNS_CHANNEL),
                std::make_shared<pb::SceneService_Stub>(new RpcClient::Channel(*channel), pb::SceneService::STUB_OWNS_CHANNEL));
        }
        addr2stubs.insert(std::make_pair(addr, stubPair));

        for (uint16_t serverId : address.serverIds) {
            auto iter1 = stubs_.find(serverId);
            if (iter1 != stubs_.end()) {
                if (iter1->second.rpcAddr == addr) {
                    stubs.insert(std::make_pair(serverId, iter1->second));
                    continue;
                }
            }

            StubInfo stubInfo = {
                addr,
                address.type,
                stubPair.first,
                stubPair.second
            };
            stubs.insert(std::make_pair(serverId, std::move(stubInfo)));
        }
    }
    
    addr2stubs_ = addr2stubs;

    {
        LockGuard lock(stubsLock_);
        stubs_ = stubs;
        stubChangeNum_++;
    }
    
    return true;
}

std::shared_ptr<pb::GameService_Stub> SceneClient::getGameServiceStub(ServerId sid) {
    refreshStubs();
    auto iter = t_stubs_.find(sid);
    if (iter == t_stubs_.end()) {
        return nullptr;
    }

    return iter->second.gameServiceStub;
}

std::shared_ptr<pb::SceneService_Stub> SceneClient::getSceneServiceStub(ServerId sid) {
    refreshStubs();
    auto iter = t_stubs_.find(sid);
    if (iter == t_stubs_.end()) {
        return nullptr;
    }

    return iter->second.sceneServiceStub;
}

void SceneClient::refreshStubs() {
    if (t_stubChangeNum_ != stubChangeNum_) {
        {
            LockGuard lock(stubsLock_);

            if (t_stubChangeNum_ == stubChangeNum_) {
                return;
            }

            t_stubs_.clear();
            t_stubs_ = stubs_;
            t_stubChangeNum_ = stubChangeNum_;
        }
    }
}
#endif
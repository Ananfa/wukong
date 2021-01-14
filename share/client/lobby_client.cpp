/*
 * Created by Xianke Liu on 2020/1/11.
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

#include "lobby_client.h"
#include "corpc_controller.h"
#include "string_utils.h"

using namespace wukong;

std::map<ServerId, LobbyClient::StubInfo> LobbyClient::_stubs;
std::mutex LobbyClient::_stubsLock;
std::atomic<uint32_t> LobbyClient::_stubChangeNum(0);
thread_local uint32_t LobbyClient::_t_stubChangeNum = 0;
thread_local std::map<ServerId, LobbyClient::StubInfo> LobbyClient::_t_stubs;

static void callDoneHandle(::google::protobuf::Message *request, Controller *controller) {
    delete controller;
    delete request;
}

bool LobbyClient::loadRole(ServerId sid, RoleId roleId) {
    // TODO:
}

std::vector<LobbyClient::ServerInfo> LobbyClient::getServerInfos() {
    std::vector<ServerInfo> infos;

    refreshStubs();
    std::map<ServerId, StubInfo> localStubs = _t_stubs;
    
    // 清除原来的信息
    infos.reserve(localStubs.size());
    // 利用(总在线人数 - 服务器在线人数)作为分配服务器的权重
    uint32_t totalCount = 0;
    for (auto &kv : localStubs) {
        corpc::Void *request = new corpc::Void();
        pb::Uint32Value *response = new pb::Uint32Value();
        Controller *controller = new Controller();
        kv.second.lobbyServiceStub->getOnlineCount(controller, request, response, nullptr);
        
        if (controller->Failed()) {
            ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
            continue;
        } else {
            infos.push_back({kv.first, response->value(), 0});
            totalCount += response->value();
        }
        
        delete controller;
        delete response;
        delete request;
    }
    
    for (auto &info : infos) {
        info.weight = totalCount - info.count;
    }

    return infos;
}

void LobbyClient::forward(ServerId sid, int16_t type, uint16_t tag, const std::vector<RoleId> &roleIds, const std::string &msg) {
    std::shared_ptr<pb::GameService_Stub> stub = getGameServiceStub(sid);
    
    if (!stub) {
        ERROR_LOG("LobbyClient server %d stub not avaliable, waiting.\n", sid);
        return;
    }
    
    pb::ForwardRequest *request = new pb::ForwardRequest();
    Controller *controller = new Controller();
    request->set_type(type);

    if (tag != 0) {
        request->set_tag(tag);
    }

    for (auto it = roleIds.begin(); it != roleIds.end(); it++) {
        request->add_ids(*it);
    }
    
    if (!msg.empty()) {
        request->set_rawmsg(msg);
    }
    
    stub->forward(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

bool LobbyClient::setServers(const std::map<ServerId, AddressInfo>& addresses) {
    if (!_client) {
        ERROR_LOG("LobbyClient not init\n");
        return false;
    }

    std::map<ServerId, StubInfo> stubs;
    for (const auto& kv : addresses) {
        ServerId sid = kv.first;
        const AddressInfo& address = kv.second;
        auto iter1 = _stubs.find(sid);
        if (iter1 != _stubs.end()) {
            if (iter1->second.ip == address.ip && iter1->second.port == address.port) {
                stubs.insert(std::make_pair(sid, iter1->second));
                continue;
            }
        }

        RpcClient::Channel *channel = new RpcClient::Channel(_client, address.ip, address.port, 1);

        StubInfo stubInfo = {
            address.ip,
            address.port,
            std::make_shared<pb::GameService_Stub>(channel, pb::GameService::STUB_OWNS_CHANNEL),
            std::make_shared<pb::LobbyService_Stub>(new RpcClient::Channel(*channel), pb::GameService::STUB_OWNS_CHANNEL)
        };
        stubs.insert(std::make_pair(sid, std::move(stubInfo)));
    }
    
    {
        std::unique_lock<std::mutex> lock(_stubsLock);
        _stubs = stubs;
        _stubChangeNum++;
    }
    
    return true;
}

std::shared_ptr<pb::GameService_Stub> LobbyClient::getGameServiceStub(ServerId sid) {
    refreshStubs();
    auto iter = _t_stubs.find(sid);
    if (iter == _t_stubs.end()) {
        return nullptr;
    }

    return iter->second.gameServiceStub;
}

std::shared_ptr<pb::LobbyService_Stub> LobbyClient::getLobbyServiceStub(ServerId sid) {
    refreshStubs();
    auto iter = _t_stubs.find(sid);
    if (iter == _t_stubs.end()) {
        return nullptr;
    }

    return iter->second.lobbyServiceStub;
}

void LobbyClient::refreshStubs() {
    if (_t_stubChangeNum != _stubChangeNum) {
        _t_stubs.clear();
        {
            std::unique_lock<std::mutex> lock(_stubsLock);
            _t_stubs = _stubs;
            _t_stubChangeNum = _stubChangeNum;
        }
    }
}

bool LobbyClient::parseAddress(const std::string &input, AddressInfo &addressInfo) {
    std::vector<std::string> output;
    StringUtils::split(input, "|", output);
    
    if (output.size() != 2) return false;

    std::vector<std::string> output1;
    StringUtils::split(output[1], ":", output1);
    if (output1.size() != 2) return false;

    addressInfo.id = std::stoi(output[0]);
    addressInfo.ip = output1[0];
    addressInfo.port = std::stoi(output1[1]);

    return true;
}
/*
 * Created by Xianke Liu on 2024/7/19.
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

#include "lobby_agent.h"

using namespace corpc;
using namespace wukong;

void LobbyAgent::setStub(const pb::ServerInfo &serverInfo) {
    auto it = stubInfos_.find(serverInfo.server_id());
    if (it != stubInfos_.end()) {
        // 若地址改变则重建stub
        if (serverInfo.rpc_host() == it->second.info.rpc_host() && serverInfo.rpc_port() == it->second.info.rpc_port()) {
            it->second.info = serverInfo;
            return;
        }

        it->second.info = serverInfo;

        if (client_) {
            it->second.stub = std::make_shared<pb::LobbyService_Stub>(new RpcClient::Channel(client_, serverInfo.rpc_host(), serverInfo.rpc_port(), 1), google::protobuf::Service::STUB_OWNS_CHANNEL);
        }
        
        return;
    }

    if (client_) {
        stubInfos_.insert(std::make_pair(serverInfo.server_id(), StubInfo{serverInfo, std::make_shared<pb::LobbyService_Stub>(new RpcClient::Channel(client_, serverInfo.rpc_host(), serverInfo.rpc_port(), 1), google::protobuf::Service::STUB_OWNS_CHANNEL)}));
    } else {
        stubInfos_.insert(std::make_pair(serverInfo.server_id(), StubInfo{info:serverInfo}));
    }
}

void LobbyAgent::shutdown() {
    auto stubInfos = stubInfos_;
    
    for (const auto& kv : stubInfos) {
        corpc::Void *request = new corpc::Void();
        
        auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(kv.second.stub);
        stub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));
    }
}

void LobbyAgent::forwardIn(ServerId sid, int32_t type, uint16_t tag, RoleId roleId, std::shared_ptr<std::string> &rawMsg) {
    auto it = stubInfos_.find(sid);
    if (it == stubInfos_.end()) {
        ERROR_LOG("LobbyAgent::forwardIn -- server %d stub not avaliable, waiting.\n", sid);
        return;
    }

    auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(it->second.stub);

    pb::ForwardInRequest *request = new pb::ForwardInRequest();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_type(type);

    if (tag != 0) {
        request->set_tag(tag);
    }

    request->set_roleid(roleId);
    
    if (rawMsg && !rawMsg->empty()) {
        request->set_rawmsg(rawMsg->c_str());
    }
    
    stub->forwardIn(controller, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request, controller));
}

bool LobbyAgent::loadRole(ServerId sid, RoleId roleId, ServerId gwId) {
    bool ret = false;
    auto it = stubInfos_.find(sid);
    if (it == stubInfos_.end()) {
        ERROR_LOG("LobbyAgent::loadRole -- server %d stub not avaliable, waiting.\n", sid);
        return ret;
    }

    auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(it->second.stub);

    pb::LoadRoleRequest *request = new pb::LoadRoleRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_roleid(roleId);
    request->set_gatewayid(gwId);
    stub->loadRole(controller, request, response, nullptr);

    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else {
        ret = response->value();
    }

    delete controller;
    delete response;
    delete request;

    return ret;
}

void LobbyAgent::enterGame(ServerId sid, RoleId roleId, const std::string &lToken, ServerId gwId) {
    auto it = stubInfos_.find(sid);
    if (it == stubInfos_.end()) {
        ERROR_LOG("LobbyAgent::enterGame -- server %d stub not avaliable, waiting.\n", sid);
        return;
    }

    auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(it->second.stub);

    pb::EnterGameRequest *request = new pb::EnterGameRequest();
    Controller *controller = new Controller();
    request->set_roleid(roleId);
    request->set_ltoken(lToken);
    request->set_gatewayid(gwId);

    stub->enterGame(controller, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request, controller));
}

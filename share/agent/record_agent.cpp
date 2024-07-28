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

#include "record_agent.h"

using namespace corpc;
using namespace wukong;

void RecordAgent::setStub(const pb::ServerInfo &serverInfo) {
    auto it = stubInfos_.find(serverInfo.server_id());
    if (it != stubInfos_.end()) {
        // 若地址改变则重建stub
        if (serverInfo.rpc_host() == it->second.info.rpc_host() && serverInfo.rpc_port() == it->second.info.rpc_port()) {
            it->second.info = serverInfo;
            return;
        }

        it->second.info = serverInfo;

        if (client_) {
            it->second.stub = std::make_shared<pb::RecordService_Stub>(new RpcClient::Channel(client_, serverInfo.rpc_host(), serverInfo.rpc_port(), 1), google::protobuf::Service::STUB_OWNS_CHANNEL);
        }
        
        return;
    }

    if (client_) {
        stubInfos_.insert(std::make_pair(serverInfo.server_id(), StubInfo{serverInfo, std::make_shared<pb::RecordService_Stub>(new RpcClient::Channel(client_, serverInfo.rpc_host(), serverInfo.rpc_port(), 1), google::protobuf::Service::STUB_OWNS_CHANNEL)}));
    } else {
        stubInfos_.insert(std::make_pair(serverInfo.server_id(), StubInfo{info:serverInfo}));
    }
}

void RecordAgent::shutdown() {
    if (!client_) {
        ERROR_LOG("RecordAgent::shutdown -- rpc client is NULL.\n");
        return;
    }

    auto stubInfos = stubInfos_;
    
    for (const auto& kv : stubInfos) {
        corpc::Void *request = new corpc::Void();
        
        auto stub = std::static_pointer_cast<pb::RecordService_Stub>(kv.second.stub);
        stub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));
    }
}

bool RecordAgent::loadRoleData(ServerId sid, RoleId roleId, UserId &userId, const std::string &lToken, ServerId &serverId, std::string &roleData) {
    if (!client_) {
        ERROR_LOG("RecordAgent::loadRoleData -- rpc client is NULL.\n");
        return false;
    }

    auto it = stubInfos_.find(sid);
    if (it == stubInfos_.end()) {
        ERROR_LOG("RecordClient::loadRoleData -- server %d stub not avaliable, waiting.\n", sid);
        return false;
    }

    auto stub = std::static_pointer_cast<pb::RecordService_Stub>(it->second.stub);

    pb::LoadRoleDataRequest *request = new pb::LoadRoleDataRequest();
    pb::LoadRoleDataResponse *response = new pb::LoadRoleDataResponse();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_ltoken(lToken);
    request->set_roleid(roleId);
    stub->loadRoleData(controller, request, response, nullptr);

    bool result = false;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else if (response->errcode() == 0) {
        result = true;
        serverId = response->serverid();
        userId = response->userid();
        roleData = response->data();
    }

    delete controller;
    delete response;
    delete request;

    return result;
}

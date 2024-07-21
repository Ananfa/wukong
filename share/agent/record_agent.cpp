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

void RecordAgent::addStub(ServerId sid, const std::string &host, int32_t port) {
    auto it = stubs_.find(sid);
    if (it != stubs_.end()) {
        // 若地址改变则重建stub
        auto stub = std::static_pointer_cast<pb::RecordService_Stub>(it->second);

        auto channel = (RpcClient::Channel*)stub->channel();

        if (channel->getHost() == host && channel->getPort() == port) {
            return;
        }

        stubs_.erase(it);
    }

    auto stub = std::make_shared<pb::RecordService_Stub>(new RpcClient::Channel(client_, host, port, 1), google::protobuf::Service::STUB_OWNS_CHANNEL);
    stubs_.insert(std::make_pair(sid, stub));
}

void RecordAgent::shutdown() {
    auto stubs = stubs_;
    
    for (const auto& kv : stubs) {
        corpc::Void *request = new corpc::Void();
        
        auto stub = std::static_pointer_cast<pb::RecordService_Stub>(kv.second);
        stub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));
    }
}

bool RecordAgent::loadRoleData(ServerId sid, RoleId roleId, UserId &userId, const std::string &lToken, ServerId &serverId, std::string &roleData) {
    auto it = stubs_.find(sid);
    if (it == stubs_.end()) {
        ERROR_LOG("RecordClient::loadRoleData -- server %d stub not avaliable, waiting.\n", sid);
        return false;
    }

    auto stub = std::static_pointer_cast<pb::RecordService_Stub>(it->second);

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

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

void LobbyAgent::addStub(ServerId sid, const std::string &host, int32_t port) {
    auto it = stubs_.find(sid);
    if (it != stubs_.end()) {
        // 若地址改变则重建stub
        auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(it->second);

        auto channel = (RpcClient::Channel*)stub->channel();

        if (channel->getHost() == host && channel->getPort() == port) {
            return;
        }

        stubs_.erase(it);
    }

    auto stub = std::make_shared<pb::LobbyService_Stub>(new RpcClient::Channel(client_, host, port, 1), google::protobuf::Service::STUB_OWNS_CHANNEL);
    stubs_.insert(std::make_pair(sid, stub));
}

void LobbyAgent::shutdown() {
    auto stubs = stubs_;
    
    for (const auto& kv : stubs) {
        corpc::Void *request = new corpc::Void();
        
        auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(kv.second);
        stub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));
    }
}

void LobbyAgent::forwardIn(ServerId sid, int16_t type, uint16_t tag, RoleId roleId, const std::string &rawMsg) {
    auto it = stubs_.find(sid);
    if (it == stubs_.end()) {
        ERROR_LOG("LobbyAgent::forwardIn -- server %d stub not avaliable, waiting.\n", sid);
        return;
    }

    auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(it->second);

    pb::ForwardInRequest *request = new pb::ForwardInRequest();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_type(type);

    if (tag != 0) {
        request->set_tag(tag);
    }

    request->set_roleid(roleId);
    
    if (!rawMsg.empty()) {
        request->set_rawmsg(rawMsg);
    }
    
    stub->forwardIn(controller, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request, controller));
}

bool LobbyAgent::loadRole(ServerId sid, RoleId roleId, ServerId gwId) {
    bool ret = false;
    auto it = stubs_.find(sid);
    if (it == stubs_.end()) {
        ERROR_LOG("LobbyAgent::loadRole -- server %d stub not avaliable, waiting.\n", sid);
        return ret;
    }

    auto stub = std::static_pointer_cast<pb::LobbyService_Stub>(it->second);

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

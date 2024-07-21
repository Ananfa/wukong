/*
 * Created by Xianke Liu on 2024/7/11.
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

#include "gateway_agent.h"

using namespace corpc;
using namespace wukong;

void GatewayAgent::addStub(ServerId sid, const std::string &host, int32_t port) {
    auto it = stubs_.find(sid);
    if (it != stubs_.end()) {
        // 若地址改变则重建stub
        auto stub = std::static_pointer_cast<pb::GatewayService_Stub>(it->second);

        auto channel = (RpcClient::Channel*)stub->channel();

        if (channel->getHost() == host && channel->getPort() == port) {
            return;
        }

        stubs_.erase(it);
    }

    auto stub = std::make_shared<pb::GatewayService_Stub>(new RpcClient::Channel(client_, host, port, 1), google::protobuf::Service::STUB_OWNS_CHANNEL);
    stubs_.insert(std::make_pair(sid, stub));
}

void GatewayAgent::shutdown() {
    auto stubs = stubs_;
    
    for (const auto& kv : stubs) {
        corpc::Void *request = new corpc::Void();
        
        auto stub = std::static_pointer_cast<pb::GatewayService_Stub>(kv.second);
        stub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));
    }
}

bool GatewayAgent::kick(ServerId sid, UserId userId, const std::string &gToken) {
    auto it = stubs_.find(sid);
    if (it == stubs_.end()) {
        ERROR_LOG("GatewayAgent::kick -- server %d stub not avaliable, waiting.\n", sid);
        return false;
    }

    auto stub = std::static_pointer_cast<pb::GatewayService_Stub>(it->second);
    
    bool result = false;
    pb::KickRequest *request = new pb::KickRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_userid(userId);
    request->set_gtoken(gToken);
    stub->kick(controller, request, response, nullptr);

    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else {
        result = response->value();
    }

    delete controller;
    delete response;
    delete request;

    return result;
}

void GatewayAgent::broadcast(ServerId sid, int32_t type, uint16_t tag, const std::vector<std::pair<UserId, std::string>> &targets, const std::string &msg) {
    if (sid == 0) { // 全服广播
        if (targets.size() > 0) { // 全服广播不应设置指定目标
            WARN_LOG("GatewayAgent::broadcast -- targets list not empty when global broadcast\n");
        }

        // 全服广播
        auto stubs = stubs_;

        if (stubs.size() > 0) {
            pb::ForwardOutRequest *request = new pb::ForwardOutRequest();
            request->set_serverid(sid);
            request->set_type(type);
            request->set_tag(tag);
            for (auto &t : targets) {
                ::wukong::pb::ForwardOutTarget* target = request->add_targets();

                target->set_userid(t.first);
                target->set_ltoken(t.second);
            }
            
            if (!msg.empty()) {
                request->set_rawmsg(msg);
            }

            std::shared_ptr<::google::protobuf::Closure> donePtr(google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request), [](::google::protobuf::Closure *done) {
                done->Run();
            });

            for (auto &kv : stubs) {
                auto stub = std::static_pointer_cast<pb::GatewayService_Stub>(kv.second);
                stub->forwardOut(nullptr, request, nullptr, google::protobuf::NewCallback(callDoneHandle, donePtr));
            }
        }
    } else { // 单服广播或多播
        auto it = stubs_.find(sid);
        if (it == stubs_.end()) {
            ERROR_LOG("GatewayAgent::broadcast -- server %d stub not avaliable, waiting.\n", sid);
            return;
        }

        auto stub = std::static_pointer_cast<pb::GatewayService_Stub>(it->second);

        pb::ForwardOutRequest *request = new pb::ForwardOutRequest();
        request->set_serverid(sid);
        request->set_type(type);
        request->set_tag(tag);
        for (auto &t : targets) {
            ::wukong::pb::ForwardOutTarget* target = request->add_targets();

            target->set_userid(t.first);
            target->set_ltoken(t.second);
        }
        
        if (!msg.empty()) {
            request->set_rawmsg(msg);
        }
        
        stub->forwardOut(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));  
    }
}

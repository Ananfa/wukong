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

#include "scene_agent.h"

using namespace corpc;
using namespace wukong;

void SceneAgent::addStub(ServerId sid, const std::string &host, int32_t port) {
    auto it = stubs_.find(sid);
    if (it != stubs_.end()) {
        // 若地址改变则重建stub
        auto stub = std::static_pointer_cast<pb::SceneService_Stub>(it->second);

        auto channel = (RpcClient::Channel*)stub->channel();

        if (channel->getHost() == host && channel->getPort() == port) {
            return;
        }

        stubs_.erase(it);
    }

    auto stub = std::make_shared<pb::SceneService_Stub>(new RpcClient::Channel(client_, host, port, 1), google::protobuf::Service::STUB_OWNS_CHANNEL);
    stubs_.insert(std::make_pair(sid, stub));
}

void SceneAgent::shutdown() {
    auto stubs = stubs_;
    
    for (const auto& kv : stubs) {
        corpc::Void *request = new corpc::Void();
        
        auto stub = std::static_pointer_cast<pb::SceneService_Stub>(kv.second);
        stub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));
    }
}

void SceneAgent::forwardIn(ServerId sid, int16_t type, uint16_t tag, RoleId roleId, const std::string &rawMsg) {
    auto it = stubs_.find(sid);
    if (it == stubs_.end()) {
        ERROR_LOG("SceneAgent::forwardIn -- server %d stub not avaliable, waiting.\n", sid);
        return;
    }

    auto stub = std::static_pointer_cast<pb::SceneService_Stub>(it->second);

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

std::string SceneAgent::loadScene(ServerId sid, uint32_t defId, const std::string &sceneId, RoleId roleId, const std::string &teamId) {
    ERROR_LOG("SceneAgent::loadScene : %s\n", sceneId.c_str());
    std::string ret;

    auto it = stubs_.find(sid);
    if (it == stubs_.end()) {
        ERROR_LOG("SceneAgent::loadScene -- server %d stub not avaliable, waiting.\n", sid);
        return ret;
    }

    auto stub = std::static_pointer_cast<pb::SceneService_Stub>(it->second);

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
            ERROR_LOG("SceneAgent::loadScene -- rpc return errCode:%d.\n", response->errcode());
        }

        ret = response->sceneid();
    }

    delete controller;
    delete response;
    delete request;

    return ret;
}

void SceneAgent::enterScene(ServerId sid, const std::string &sceneId, RoleId roleId, ServerId gwId) {
    auto it = stubs_.find(sid);
    if (it == stubs_.end()) {
        ERROR_LOG("SceneAgent::enterScene -- server %d stub not avaliable, waiting.\n", sid);
        return;
    }

    auto stub = std::static_pointer_cast<pb::SceneService_Stub>(it->second);

    pb::EnterSceneRequest *request = new pb::EnterSceneRequest();
    request->set_serverid(sid);
    request->set_roleid(roleId);
    request->set_gatewayid(gwId);
    request->set_sceneid(sceneId);
    stub->enterScene(nullptr, request, nullptr, google::protobuf::NewCallback<google::protobuf::Message *>(callDoneHandle, request));
}

/*
 * Created by Xianke Liu on 2021/1/15.
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

#include "lobby_service.h"
#include "lobby_config.h"
#include "lobby_delegate.h"

using namespace wukong;


void LobbyServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                              const ::corpc::Void* request,
                              ::corpc::Void* response,
                              ::google::protobuf::Closure* done) {
    traverseInnerStubs([](ServerId sid, pb::InnerLobbyService_Stub *stub) -> bool {
        stub->shutdown(NULL, NULL, NULL, NULL);
        return true;
    });
}

void LobbyServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                    const ::corpc::Void* request,
                                    ::wukong::pb::OnlineCounts* response,
                                    ::google::protobuf::Closure* done) {
    // 注意：这里处理中间会产生协程切换，遍历的Map可能在过程中被修改，因此traverseInnerStubs方法中对map进行了镜像复制
    traverseInnerStubs([request, response](ServerId sid, pb::InnerLobbyService_Stub *stub) -> bool {
        pb::Uint32Value *resp = new pb::Uint32Value();
        corpc::Controller *ctl = new corpc::Controller();
        stub->getOnlineCount(ctl, request, resp, NULL);
        pb::OnlineCount *onlineCount = response->add_counts();
        onlineCount->set_serverid(sid);
        if (ctl->Failed()) {
            onlineCount->set_count(-1);
        } else {
            onlineCount->set_count(resp->value());
        }
        delete ctl;
        delete resp;

        return true;
    });
}

void LobbyServiceImpl::loadRole(::google::protobuf::RpcController* controller,
                              const ::wukong::pb::LoadRoleRequest* request,
                              ::wukong::pb::BoolValue* response,
                              ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->loadRole(controller, request, response, done);
}

void LobbyServiceImpl::addInnerStub(ServerId sid, pb::InnerLobbyService_Stub* stub) {
    innerStubs_.insert(std::make_pair(sid, stub));
}

pb::InnerLobbyService_Stub *LobbyServiceImpl::getInnerStub(ServerId sid) {
    auto it = innerStubs_.find(sid);

    if (it == innerStubs_.end()) {
        return nullptr;
    }

    return it->second;
}

void LobbyServiceImpl::traverseInnerStubs(std::function<bool(ServerId, pb::InnerLobbyService_Stub*)> handle) {
    for (auto &pair : innerStubs_) {
        if (!handle(pair.first, pair.second)) {
            return;
        }
    }
}

void InnerLobbyServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                const ::corpc::Void* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    manager_->shutdown();
}

void InnerLobbyServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                      const ::corpc::Void* request,
                                      ::wukong::pb::Uint32Value* response,
                                      ::google::protobuf::Closure* done) {
    response->set_value(manager_->roleCount());
}

void InnerLobbyServiceImpl::loadRole(::google::protobuf::RpcController* controller,
                                const ::wukong::pb::LoadRoleRequest* request,
                                ::wukong::pb::BoolValue* response,
                                ::google::protobuf::Closure* done) {
    RoleId roleId = request->roleid();
    ServerId gwId = request->gatewayid();

    // TODO: 在大厅中加载角色或在场景中加载角色
    if (!g_LobbyDelegate.getGetTargetSceneIdHandle()) {
        ERROR_LOG("InnerLobbyServiceImpl::loadRole -- lobby delegate not init\n");
        return;
    }

    std::string targetSceneId = g_LobbyDelegate.getGetTargetSceneIdHandle()(roleId);

    if (targetSceneId.empty()) {
        if (!manager_->loadRole(roleId, gwId)) {
            ERROR_LOG("InnerLobbyServiceImpl::loadRole -- role %d load failed\n", roleId);
            return;
        }

        response->set_value(true);
    } else {
        // TODO：查找场景（redis）

        // TODO: 判断场景不存在时是否加载场景（delegate）

        // TODO: 判断场景是否会自动加载角色（delegate）
    }
}

/*
 * Created by Xianke Liu on 2021/1/19.
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

#include "game_service.h"
#include "game_center.h"

using namespace wukong;

void GameServiceImpl::forwardIn(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::ForwardInRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done) {
    // 注意：该RPC定义使用了delete_in_done选项
    assert(controller == NULL);
    auto stub = getInnerStub(request->serverid());
    stub->forwardIn(controller, request, NULL, done);
}
        
void GameServiceImpl::enterGame(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::EnterGameRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done) {
    // 注意：该RPC定义使用了delete_in_done选项
    assert(controller == NULL);
    auto stub = getInnerStub(request->serverid());
    stub->enterGame(controller, request, NULL, done);
}

void GameServiceImpl::addInnerStub(ServerId sid, pb::InnerGameService_Stub* stub) {
    _innerStubs.insert(std::make_pair(sid, stub));
}

pb::InnerGameService_Stub *GameServiceImpl::getInnerStub(ServerId sid) {
    auto it = _innerStubs.find(sid);

    if (it == _innerStubs.end()) {
        return nullptr;
    }

    return it->second;
}

void GameServiceImpl::traverseInnerStubs(std::function<bool(ServerId, pb::InnerGameService_Stub*)> handle) {
    for (auto &pair : _innerStubs) {
        if (!handle(pair.first, pair.second)) {
            return;
        }
    }
}

void InnerGameServiceImpl::forwardIn(::google::protobuf::RpcController* controller,
                                const ::wukong::pb::ForwardInRequest* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    auto obj = _manager->getGameObject(request->roleid());
    if (!obj) {
        ERROR_LOG("InnerGameServiceImpl::forwardIn -- role[%llu] game object not found\n", request->roleid());
//exit(0);
        return;
    }

    g_GameCenter.handleMessage(obj, request->type(), request->tag(), request->rawmsg());
}

void InnerGameServiceImpl::enterGame(::google::protobuf::RpcController* controller,
                                const ::wukong::pb::EnterGameRequest* request,
                                ::corpc::Void* response,
                                ::google::protobuf::Closure* done) {
    // 获取GameObject
    auto obj = _manager->getGameObject(request->roleid());
    if (!obj) {
        ERROR_LOG("InnerGameServiceImpl::enterGame -- role[%llu] game object not found\n", request->roleid());
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("InnerGameServiceImpl::enterGame -- role[%llu] ltoken not match\n", request->roleid());
        return;
    }

    // 刷新gateway stub
    if (!obj->setGatewayServerStub(request->gatewayid())) {
        ERROR_LOG("InnerGameServiceImpl::enterGame -- role[%llu] set gateway stub failed\n", request->roleid());
        return;
    }

    obj->enterGame();
}

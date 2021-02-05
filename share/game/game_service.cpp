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

using namespace wukong;

void GameServiceImpl::forwardIn(::google::protobuf::RpcController* controller,
                                     const ::wukong::pb::ForwardInRequest* request,
                                     ::corpc::Void* response,
                                     ::google::protobuf::Closure* done) {
    // TODO: 设计游戏对象的消息处理机制（是否使用注册消息处理）
}

void GameServiceImpl::enterGame(::google::protobuf::RpcController* controller,
                                     const ::wukong::pb::EnterGameRequest* request,
                                     ::corpc::Void* response,
                                     ::google::protobuf::Closure* done) {
    // 获取GameObject
    auto obj = _manager->getGameObject(request->roleid());
    if (!obj) {
        ERROR_LOG("GameServiceImpl::enterGame -- game object not found\n");
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("GameServiceImpl::enterGame -- ltoken not match\n");
        return;
    }

    // 刷新gateway stub
    if (!obj->setGatewayServerStub(request->gatewayid())) {
        ERROR_LOG("GameServiceImpl::enterGame -- set gateway stub failed\n");
        return;
    }

    obj->onEnterGame();
}

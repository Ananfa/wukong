/*
 * Created by Xianke Liu on 2020/12/11.
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

#include "gateway_service.h"
#include "gateway_config.h"

using namespace wukong;

void GatewayServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                  const ::corpc::Void* request,
                                  ::corpc::Void* response,
                                  ::google::protobuf::Closure* done) {
    // TODO: shutdown gateway -- kick out all player and forbit new connection
}

void GatewayServiceImpl::kick(::google::protobuf::RpcController* controller,
                              const ::wukong::pb::Uint32Value* request,
                              ::wukong::pb::BoolValue* response,
                              ::google::protobuf::Closure* done) {
    // TODO: kick out one player
}

void GatewayServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                        const ::corpc::Void* request,
                                        ::wukong::pb::Uint32Value* response,
                                        ::google::protobuf::Closure* done) {
    // TODO: get online player count
}

void GatewayServiceImpl::forward(::google::protobuf::RpcController* controller,
                                 const ::wukong::pb::ForwardRequest* request,
                                 ::corpc::Void* response,
                                 ::google::protobuf::Closure* done) {
    // TODO: forward message to player clients
}

void GatewayServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                                   const ::wukong::pb::HeartbeatRequest* request,
                                   ::corpc::Void* response,
                                   ::google::protobuf::Closure* done) {
    std::shared_ptr<RouteObject> ro = _manager->getRouteObject(request->userid());
    if (ro) {
        struct timeval t;
        gettimeofday(&t, NULL);
        ro->_gameObjectHeartbeatExpire = t.tv_sec + 60;
        ro->setGameServerStub(request->servertype(), request->serverid());
    }

}
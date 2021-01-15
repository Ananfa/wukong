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
    _manager->shutdown();
}

void GatewayServiceImpl::kick(::google::protobuf::RpcController* controller,
                              const ::wukong::pb::Uint32Value* request,
                              ::wukong::pb::BoolValue* response,
                              ::google::protobuf::Closure* done) {
    response->set_value(_manager->removeRouteObject(request->value()));
}

void GatewayServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                        const ::corpc::Void* request,
                                        ::wukong::pb::Uint32Value* response,
                                        ::google::protobuf::Closure* done) {
    response->set_value(_manager->getRouteObjectNum());
}

void GatewayServiceImpl::forwardOut(::google::protobuf::RpcController* controller,
                                 const ::wukong::pb::ForwardOutRequest* request,
                                 ::corpc::Void* response,
                                 ::google::protobuf::Closure* done) {
    for (int i = 0; i < request->targets_size(); i++) {
        // 不论路由对象是否断线都进行转发，消息会存到消息缓存中
        auto ro = _manager->getRouteObject(request->targets(i).userid());
        if (!ro) {
            DEBUG_LOG("GatewayServiceImpl::forwardOut -- route object not found\n");
            continue;
        }

        std::shared_ptr<std::string> msg(new std::string(request->rawmsg()));
        ro->getConn()->send(request->type(), true, true, request->tag(), msg);
    }
}

void GatewayServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                                   const ::wukong::pb::HeartbeatRequest* request,
                                   ::wukong::pb::BoolValue* response,
                                   ::google::protobuf::Closure* done) {
    std::shared_ptr<RouteObject> ro = _manager->getRouteObject(request->userid());
    if (!ro) {
        ERROR_LOG("GatewayServiceImpl::heartbeat -- route object not found\n");
        return;
    }

    if (ro->getRoleId() == request->roleid()) {
        ERROR_LOG("GatewayServiceImpl::heartbeat -- roleid not match\n");
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    ro->_gameObjectHeartbeatExpire = t.tv_sec + 60;
    ro->setGameServerStub(request->servertype(), request->serverid());
    response->set_value(true);
}
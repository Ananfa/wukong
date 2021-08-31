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
                              const ::wukong::pb::KickRequest* request,
                              ::wukong::pb::BoolValue* response,
                              ::google::protobuf::Closure* done) {
    DEBUG_LOG("GatewayServiceImpl::kick -- user[%d]\n", request->userid());
    auto obj = _manager->getGatewayObject(request->userid());
    if (obj && obj->getGToken() == request->gtoken()) {
        response->set_value(_manager->removeGatewayObject(request->userid()));
    } else {
        if (!obj) {
            ERROR_LOG("GatewayServiceImpl::kick -- obj not exist\n");
        } else {
            ERROR_LOG("GatewayServiceImpl::kick -- user[%d] token not match, %s -- %s\n", request->userid(), obj->getGToken().c_str(), request->gtoken().c_str());
        }
        
        response->set_value(false);
    }
}

void GatewayServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                        const ::corpc::Void* request,
                                        ::wukong::pb::Uint32Value* response,
                                        ::google::protobuf::Closure* done) {
    response->set_value(_manager->getGatewayObjectNum());
}

void GatewayServiceImpl::forwardOut(::google::protobuf::RpcController* controller,
                                    const ::wukong::pb::ForwardOutRequest* request,
                                    ::corpc::Void* response,
                                    ::google::protobuf::Closure* done) {
    for (int i = 0; i < request->targets_size(); i++) {
        // 不论网关对象是否断线都进行转发，消息会存到消息缓存中
        const ::wukong::pb::ForwardOutTarget& target = request->targets(i);
        auto obj = _manager->getGatewayObject(target.userid());
        if (!obj) {
            DEBUG_LOG("GatewayServiceImpl::forwardOut -- gateway object not found\n");
            continue;
        }

        if (obj->getLToken() != target.ltoken()) {
            DEBUG_LOG("GatewayServiceImpl::forwardOut -- gateway object ltoken not match\n");
            continue;
        }

        std::shared_ptr<std::string> msg(new std::string(request->rawmsg()));
        obj->getConn()->send(request->type(), true, true, request->tag(), msg);
    }
}

void GatewayServiceImpl::setGameObjectPos(::google::protobuf::RpcController* controller,
                                          const ::wukong::pb::SetGameObjectPosRequest* request,
                                          ::wukong::pb::BoolValue* response,
                                          ::google::protobuf::Closure* done) {
    std::shared_ptr<GatewayObject> obj = _manager->getGatewayObject(request->userid());
    if (!obj) {
        ERROR_LOG("GatewayServiceImpl::setGameObjectPos -- gateway object not found\n");
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("GatewayServiceImpl::setGameObjectPos -- ltoken not match\n");
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    obj->_gameObjectHeartbeatExpire = t.tv_sec + 60;
    obj->setGameServerStub(request->servertype(), request->serverid());
    response->set_value(true);
}

void GatewayServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                                   const ::wukong::pb::GSHeartbeatRequest* request,
                                   ::wukong::pb::BoolValue* response,
                                   ::google::protobuf::Closure* done) {
    std::shared_ptr<GatewayObject> obj = _manager->getGatewayObject(request->userid());
    if (!obj) {
        ERROR_LOG("GatewayServiceImpl::heartbeat -- gateway object not found\n");
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("GatewayServiceImpl::heartbeat -- ltoken not match, local ltoken:%d, remote ltoken:%d\n", obj->getLToken(), request->ltoken());
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    obj->_gameObjectHeartbeatExpire = t.tv_sec + 60;
    response->set_value(true);
}
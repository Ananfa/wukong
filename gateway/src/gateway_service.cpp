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
#include "gateway_server.h"
#include "gateway_object_manager.h"

using namespace wukong;

void GatewayServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                     const ::corpc::Void* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    g_GatewayObjectManager.shutdown();
}

void GatewayServiceImpl::kick(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::KickRequest* request,
                     ::wukong::pb::BoolValue* response,
                     ::google::protobuf::Closure* done) {
    DEBUG_LOG("GatewayServiceImpl::kick -- user[%d]\n", request->userid());
    auto obj = g_GatewayObjectManager.getGatewayObject(request->userid());
    if (obj && obj->getGToken() == request->gtoken()) {
        response->set_value(g_GatewayObjectManager.removeGatewayObject(request->userid()));
    } else {
        if (!obj) {
            ERROR_LOG("GatewayServiceImpl::kick -- obj not exist\n");
        } else {
            ERROR_LOG("GatewayServiceImpl::kick -- user[%llu] token not match, %s -- %s\n", request->userid(), obj->getGToken().c_str(), request->gtoken().c_str());
        }
        
        response->set_value(false);
    }
}

void GatewayServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                     const ::corpc::Void* request,
                     ::wukong::pb::OnlineCount* response,
                     ::google::protobuf::Closure* done) {
    response->set_count(g_GatewayObjectManager.getGatewayObjectNum());
}

void GatewayServiceImpl::forwardOut(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::ForwardOutRequest* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    std::shared_ptr<std::string> msg(new std::string(request->rawmsg())); // 进行一次数据拷贝

    if (request->targets_size() == 0) {
        // 广播给所有在线玩家（这里不包括断线中的玩家）
        g_GatewayObjectManager.traverseConnectedGatewayObject([request, msg](std::shared_ptr<GatewayObject> &obj) -> bool {
            obj->getConn()->send(request->type(), true, true, true, 0, msg);
            return true;
        });
    } else {
        // 发送给指定玩家（这里包括断线中的玩家）
        for (int i = 0; i < request->targets_size(); i++) {
            // 不论网关对象是否断线都进行转发，消息会存到消息缓存中
            const ::wukong::pb::ForwardOutTarget& target = request->targets(i);
            auto obj = g_GatewayObjectManager.getGatewayObject(target.userid());
            if (!obj) {
                WARN_LOG("GatewayServiceImpl::forwardOut -- user[%llu] gateway object not found\n", target.userid());
                continue;
            }

            if (obj->getLToken() != target.ltoken()) {
                WARN_LOG("GatewayServiceImpl::forwardOut -- user[%llu] gateway object ltoken not match\n", target.userid());
                continue;
            }

            //DEBUG_LOG("InnerGatewayServiceImpl::forwardOut -- fd: %d\n", obj->getConn()->getfd());
            obj->getConn()->send(request->type(), true, true, true, request->tag(), msg);
        }
    }
}

void GatewayServiceImpl::setGameObjectPos(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::SetGameObjectPosRequest* request,
                     ::wukong::pb::BoolValue* response,
                     ::google::protobuf::Closure* done) {
    std::shared_ptr<GatewayObject> obj = g_GatewayObjectManager.getGatewayObject(request->userid());
    if (!obj) {
        // 注意：正常情况下这里是会进入的，玩家刚进入游戏时的流程里面会进到这里
        //ERROR_LOG("GatewayServiceImpl::setGameObjectPos -- user[%llu] gateway object not found\n", request->userid());
        return;
    }

    // 场景切换时重建gameobj对象时，ltoken会重新生成，此时会与session中记录的值不一致，这里不应判断ltoken值
    //if (obj->getLToken() != request->ltoken()) {
    //    ERROR_LOG("GatewayServiceImpl::setGameObjectPos -- user[%llu] ltoken not match\n", request->userid());
    //    return;
    //}

    if (obj->getRoleId() != request->roleid()) {
        ERROR_LOG("GatewayServiceImpl::setGameObjectPos -- user[%llu] roleid not match\n", request->userid());
        return;
    }

    // 重置ltoken
    obj->setLToken(request->ltoken());

    struct timeval t;
    gettimeofday(&t, NULL);
    obj->gameObjectHeartbeatExpire_ = t.tv_sec + 60;
    obj->setGameServerStub(request->gstype(), request->gsid());
    response->set_value(true);
}

void GatewayServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::GSHeartbeatRequest* request,
                     ::wukong::pb::BoolValue* response,
                     ::google::protobuf::Closure* done) {
    std::shared_ptr<GatewayObject> obj = g_GatewayObjectManager.getGatewayObject(request->userid());
    if (!obj) {
        WARN_LOG("GatewayServiceImpl::heartbeat -- user[%llu] gateway object not found\n", request->userid());
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("GatewayServiceImpl::heartbeat -- user[%llu] role[%llu] ltoken not match, local ltoken:%s, remote ltoken:%s\n", obj->getUserId(), obj->getRoleId(), obj->getLToken().c_str(), request->ltoken().c_str());
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    obj->gameObjectHeartbeatExpire_ = t.tv_sec + 60;
    response->set_value(true);
}

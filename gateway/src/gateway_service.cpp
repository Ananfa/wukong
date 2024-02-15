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

using namespace wukong;

void GatewayServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                     const ::corpc::Void* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    traverseInnerStubs([](ServerId sid, pb::InnerGatewayService_Stub *stub) -> bool {
        stub->shutdown(NULL, NULL, NULL, NULL);
        return true;
    });
}

void GatewayServiceImpl::kick(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::KickRequest* request,
                     ::wukong::pb::BoolValue* response,
                     ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->kick(controller, request, response, done);
}

void GatewayServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                     const ::corpc::Void* request,
                     ::wukong::pb::OnlineCounts* response,
                     ::google::protobuf::Closure* done) {
    // 注意：这里处理中间会产生协程切换，遍历的Map可能在过程中被修改，因此traverseInnerStubs方法中对map进行了镜像复制
    traverseInnerStubs([request, response](ServerId sid, pb::InnerGatewayService_Stub *stub) -> bool {
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

void GatewayServiceImpl::forwardOut(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::ForwardOutRequest* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    // 注意：forwardOut接口在pb接口定义中将delete_in_done选项设置为true，当done->Run()调用时才销毁controller和request
    assert(controller == NULL);
    if (request->serverid() == 0) {
        // 广播
        if (request->targets_size() > 0) {
            WARN_LOG("GatewayServiceImpl::forwardOut -- targets list not empty when broadcast\n");
        }

        // 通过shared_ptr在所有通知都处理完成时才调用done->Run()方法
        std::shared_ptr<::google::protobuf::Closure> donePtr(done, [](::google::protobuf::Closure *done) {
            done->Run();
        });

        traverseInnerStubs([request, donePtr](ServerId sid, pb::InnerGatewayService_Stub* stub) -> bool {
            // 因为forwardOut调用不会等返回，因此需要再用NewCallback保持住donePtr，等到处理完成后才释放
            stub->forwardOut(NULL, request, NULL, google::protobuf::NewCallback(callDoneHandle, donePtr));
            return true;
        });
    } else {
        // 单播或多播
        auto stub = getInnerStub(request->serverid());

        stub->forwardOut(controller, request, NULL, done);
    }
}

void GatewayServiceImpl::setGameObjectPos(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::SetGameObjectPosRequest* request,
                     ::wukong::pb::BoolValue* response,
                     ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->setGameObjectPos(controller, request, response, done);
}

void GatewayServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::GSHeartbeatRequest* request,
                     ::wukong::pb::BoolValue* response,
                     ::google::protobuf::Closure* done) {
    auto stub = getInnerStub(request->serverid());
    stub->heartbeat(controller, request, response, done);
}

void GatewayServiceImpl::addInnerStub(ServerId sid, pb::InnerGatewayService_Stub* stub) {
    innerStubs_.insert(std::make_pair(sid, stub));
}

pb::InnerGatewayService_Stub *GatewayServiceImpl::getInnerStub(ServerId sid) {
    auto it = innerStubs_.find(sid);

    if (it == innerStubs_.end()) {
        return nullptr;
    }

    return it->second;
}

void GatewayServiceImpl::traverseInnerStubs(std::function<bool(ServerId, pb::InnerGatewayService_Stub*)> handle) {
    for (auto &pair : innerStubs_) {
        if (!handle(pair.first, pair.second)) {
            return;
        }
    }
}

void InnerGatewayServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                                  const ::corpc::Void* request,
                                  ::corpc::Void* response,
                                  ::google::protobuf::Closure* done) {
    manager_->shutdown();
}

void InnerGatewayServiceImpl::kick(::google::protobuf::RpcController* controller,
                              const ::wukong::pb::KickRequest* request,
                              ::wukong::pb::BoolValue* response,
                              ::google::protobuf::Closure* done) {
    DEBUG_LOG("InnerGatewayServiceImpl::kick -- user[%d]\n", request->userid());
    auto obj = manager_->getGatewayObject(request->userid());
    if (obj && obj->getGToken() == request->gtoken()) {
        response->set_value(manager_->removeGatewayObject(request->userid()));
    } else {
        if (!obj) {
            ERROR_LOG("InnerGatewayServiceImpl::kick -- obj not exist\n");
        } else {
            ERROR_LOG("InnerGatewayServiceImpl::kick -- user[%llu] token not match, %s -- %s\n", request->userid(), obj->getGToken().c_str(), request->gtoken().c_str());
        }
        
        response->set_value(false);
    }
}

void InnerGatewayServiceImpl::getOnlineCount(::google::protobuf::RpcController* controller,
                                        const ::corpc::Void* request,
                                        ::wukong::pb::Uint32Value* response,
                                        ::google::protobuf::Closure* done) {
    response->set_value(manager_->getGatewayObjectNum());
}

void InnerGatewayServiceImpl::forwardOut(::google::protobuf::RpcController* controller,
                                    const ::wukong::pb::ForwardOutRequest* request,
                                    ::corpc::Void* response,
                                    ::google::protobuf::Closure* done) {
    std::shared_ptr<std::string> msg(new std::string(request->rawmsg())); // 进行一次数据拷贝

    if (request->targets_size() == 0) {
        // 广播给所有在线玩家（这里不包括断线中的玩家）
        manager_->traverseConnectedGatewayObject([request, msg](std::shared_ptr<GatewayObject> &obj) -> bool {
            obj->getConn()->send(request->type(), true, true, true, 0, msg);
            return true;
        });
    } else {
        // 发送给指定玩家（这里包括断线中的玩家）
        for (int i = 0; i < request->targets_size(); i++) {
            // 不论网关对象是否断线都进行转发，消息会存到消息缓存中
            const ::wukong::pb::ForwardOutTarget& target = request->targets(i);
            auto obj = manager_->getGatewayObject(target.userid());
            if (!obj) {
                WARN_LOG("InnerGatewayServiceImpl::forwardOut -- user[%llu] gateway object not found\n", target.userid());
                continue;
            }

            if (obj->getLToken() != target.ltoken()) {
                WARN_LOG("InnerGatewayServiceImpl::forwardOut -- user[%llu] gateway object ltoken not match\n", target.userid());
                continue;
            }

            //DEBUG_LOG("InnerGatewayServiceImpl::forwardOut -- fd: %d\n", obj->getConn()->getfd());
            obj->getConn()->send(request->type(), true, true, true, request->tag(), msg);
        }
    }
    
}

void InnerGatewayServiceImpl::setGameObjectPos(::google::protobuf::RpcController* controller,
                                          const ::wukong::pb::SetGameObjectPosRequest* request,
                                          ::wukong::pb::BoolValue* response,
                                          ::google::protobuf::Closure* done) {
    std::shared_ptr<GatewayObject> obj = manager_->getGatewayObject(request->userid());
    if (!obj) {
        // 注意：正常情况下这里是会进入的，玩家刚进入游戏时的流程里面会进到这里
        //ERROR_LOG("InnerGatewayServiceImpl::setGameObjectPos -- user[%llu] gateway object not found\n", request->userid());
        return;
    }

    // 场景切换时重建gameobj对象时，ltoken会重新生成，此时会与session中记录的值不一致，这里不应判断ltoken值
    //if (obj->getLToken() != request->ltoken()) {
    //    ERROR_LOG("InnerGatewayServiceImpl::setGameObjectPos -- user[%llu] ltoken not match\n", request->userid());
    //    return;
    //}

    if (obj->getRoleId() != request->roleid()) {
        ERROR_LOG("InnerGatewayServiceImpl::setGameObjectPos -- user[%llu] roleid not match\n", request->userid());
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

void InnerGatewayServiceImpl::heartbeat(::google::protobuf::RpcController* controller,
                                   const ::wukong::pb::GSHeartbeatRequest* request,
                                   ::wukong::pb::BoolValue* response,
                                   ::google::protobuf::Closure* done) {
    std::shared_ptr<GatewayObject> obj = manager_->getGatewayObject(request->userid());
    if (!obj) {
        WARN_LOG("InnerGatewayServiceImpl::heartbeat -- user[%llu] gateway object not found\n", request->userid());
        return;
    }

    if (obj->getLToken() != request->ltoken()) {
        ERROR_LOG("InnerGatewayServiceImpl::heartbeat -- user[%llu] role[%llu] ltoken not match, local ltoken:%s, remote ltoken:%s\n", obj->getUserId(), obj->getRoleId(), obj->getLToken().c_str(), request->ltoken().c_str());
        return;
    }

    struct timeval t;
    gettimeofday(&t, NULL);
    obj->gameObjectHeartbeatExpire_ = t.tv_sec + 60;
    response->set_value(true);
}
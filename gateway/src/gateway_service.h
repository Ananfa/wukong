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

#ifndef gateway_service_h
#define gateway_service_h

#include "corpc_controller.h"
#include "service_common.pb.h"
#include "gateway_service.pb.h"
#include "gateway_manager.h"

namespace wukong {
    class GatewayServiceImpl : public pb::GatewayService {
        // TODO: 玩家路由对象表
        // TODO: 消息处理（新建连接处理、连接关闭处理、身份校验消息）
        // TODO: 消息屏蔽相关逻辑（若屏蔽的消息带了tag，返回“屏蔽”错误消息也需带上tag）
        // TODO: 旁路消息处理
        // TODO: 接收和返回带tag号的消息（与客户端的RPC机制，消息头部增加tag号，转发消息给内部服务器时需要把tag号带上）

    public:
        GatewayServiceImpl(GatewayManager *manager): _manager(manager) {}
        
        virtual void shutdown(::google::protobuf::RpcController* controller,
                              const ::corpc::Void* request,
                              ::corpc::Void* response,
                              ::google::protobuf::Closure* done);
        
        virtual void kick(::google::protobuf::RpcController* controller,
                          const ::wukong::pb::Uint32Value* request,
                          ::wukong::pb::BoolValue* response,
                          ::google::protobuf::Closure* done);

        virtual void getOnlineCount(::google::protobuf::RpcController* controller,
                                    const ::corpc::Void* request,
                                    ::wukong::pb::Uint32Value* response,
                                    ::google::protobuf::Closure* done);

        virtual void forward(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::ForwardRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);

        virtual void heartbeat(::google::protobuf::RpcController* controller,
                               const ::wukong::pb::HeartbeatRequest* request,
                               ::corpc::Void* response,
                               ::google::protobuf::Closure* done);

    private:
        GatewayManager *_manager;
    };

}

#endif /* gateway_service_h */

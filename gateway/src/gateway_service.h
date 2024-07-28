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

#ifndef wukong_gateway_service_h
#define wukong_gateway_service_h

#include "corpc_controller.h"
#include "gateway_service.pb.h"
#include "gateway_object_manager.h"

namespace wukong {
    class GatewayServiceImpl : public pb::GatewayService {
    public:
        GatewayServiceImpl() {}
        
        virtual void shutdown(::google::protobuf::RpcController* controller,
                             const ::corpc::Void* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void kick(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::KickRequest* request,
                             ::wukong::pb::BoolValue* response,
                             ::google::protobuf::Closure* done);
        //virtual void getOnlineCount(::PROTOBUF_NAMESPACE_ID::RpcController* controller,
        //                     const ::corpc::Void* request,
        //                     ::wukong::pb::Uint32Value* response,
        //                     ::google::protobuf::Closure* done);
        virtual void forwardOut(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::ForwardOutRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        //virtual void setGameObjectPos(::google::protobuf::RpcController* controller,
        //                     const ::wukong::pb::SetGameObjectPosRequest* request,
        //                     ::wukong::pb::BoolValue* response,
        //                     ::google::protobuf::Closure* done);
        virtual void heartbeat(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::GSHeartbeatRequest* request,
                             ::wukong::pb::BoolValue* response,
                             ::google::protobuf::Closure* done);

    };
}

#endif /* wukong_gateway_service_h */

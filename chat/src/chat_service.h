/*
 * Created by Xianke Liu on 2025/5/21.
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

#ifndef wukong_chat_service_h
#define wukong_chat_service_h

#include "corpc_controller.h"
#include "chat_service.pb.h"

namespace wukong {

    class ChatServiceImpl : public pb::ChatService {
    public:
        virtual void shutdown(::google::protobuf::RpcController* controller,
                             const ::corpc::Void* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void loginChat(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LoginChatRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void logoutChat(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LogoutChatRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void enterChatRoom(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::EnterChatRoomRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void leaveChatRoom(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::LeaveChatRoomRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
        virtual void sendChatMessage(::google::protobuf::RpcController* controller,
                             const ::wukong::pb::SendChatMessageRequest* request,
                             ::corpc::Void* response,
                             ::google::protobuf::Closure* done);
    };

}

#endif /* wukong_chat_service_h */

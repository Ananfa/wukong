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

#include "chat_service.h"
#include "chat_config.h"
#include "chat_room_manager.h"
#include "message_handle_manager.h"

using namespace wukong;


void ChatServiceImpl::shutdown(::google::protobuf::RpcController* controller,
                     const ::corpc::Void* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    // TODO:
}

void ChatServiceImpl::loginChat(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::LoginChatRequest* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    // TODO:
}

void ChatServiceImpl::logoutChat(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::LogoutChatRequest* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    // TODO:
}

void ChatServiceImpl::enterChatRoom(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::EnterChatRoomRequest* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    // TODO:
}

void ChatServiceImpl::leaveChatRoom(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::LeaveChatRoomRequest* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    // TODO:
}

void ChatServiceImpl::sendChatMessage(::google::protobuf::RpcController* controller,
                     const ::wukong::pb::SendChatMessageRequest* request,
                     ::corpc::Void* response,
                     ::google::protobuf::Closure* done) {
    // TODO:
}
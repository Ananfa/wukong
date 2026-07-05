/*
 * Created by Xianke Liu on 2025/5/22.
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
#if 0
#include "chat_message_handler.h"
#include "message_handle_manager.h"
#include "common.pb.h"

using namespace demo;

void MessageHandler::registerMessages() {
    g_MessageHandleManager.registerMessage(C2S_MESSAGE_ID_CHAT, new wukong::pb::StringValue, false, EchoHandle);
    g_MessageHandleManager.registerMessage(C2S_MESSAGE_ID_ENTERSCENE, new wukong::pb::Int32Value, true, EnterSceneHandle);
}

void MessageHandler::EchoHandle(std::shared_ptr<MessageTarget> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    DEBUG_LOG("MessageHandler::EchoHandle\n");
    std::shared_ptr<MyLobbyObject> realObj = std::dynamic_pointer_cast<MyLobbyObject>(obj);
    std::shared_ptr<wukong::pb::StringValue> realMsg = std::dynamic_pointer_cast<wukong::pb::StringValue>(msg);

    DEBUG_LOG("MessageHandler::EchoHandle -- receive msg: %s\n", realMsg->value().c_str());

    // 加1点经验值
    realObj->setExp(realObj->getExp()+1);

    realObj->send(S2C_MESSAGE_ID_ECHO, tag, *realMsg);
}
#endif
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
#ifndef chat_message_handle_h
#define chat_message_handle_h

#include "chat_role.h"
#include "common.pb.h"

namespace wukong {
    // 注意：应该每个功能模块有各自的MessageHandler实现
    class ChatMessageHandler {
    public:
        static void registerMessages();

        static void ChatHandle(std::shared_ptr<MessageTarget> obj, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg);
    };
}

#endif /* chat_message_handle_h */
#endif
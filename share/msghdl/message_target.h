/*
 * Created by Xianke Liu on 2024/7/31.
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

#ifndef wukong_message_target_h
#define wukong_message_target_h

#include <memory>
#include <list>
#include <string>

#include <google/protobuf/message.h>

namespace wukong {
    class MessageTarget;

    typedef std::function<void (std::shared_ptr<MessageTarget>, uint16_t, std::shared_ptr<google::protobuf::Message>)> MessageHandle;

    class MessageTarget: public std::enable_shared_from_this<MessageTarget> {
        struct MessageInfo {
            int msgType;
            uint16_t tag;
            std::shared_ptr<google::protobuf::Message> targetMsg;
            MessageHandle handle;
            bool needHotfix;
        };

    public:
        MessageTarget(): need_wait_(false) {}
        virtual ~MessageTarget() = 0;

        std::shared_ptr<MessageTarget> getPtr() {
            return shared_from_this();
        }

        void handleMessage(int msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> &msg, MessageHandle handle, bool needCoroutine, bool needHotfix);

    private:
        void needWait(bool need_wait) { need_wait_ = need_wait; }
        bool needWait() { return need_wait_; }

        static void *handleMessageRoutine(void * arg);
        void callHotfix(int msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg);

    private:
        std::list<MessageInfo> wait_messages_;
        bool need_wait_;
    };
}

#endif /* wukong_message_target_h */

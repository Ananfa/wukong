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

#ifndef wukong_message_handle_manager_h
#define wukong_message_handle_manager_h

#include "corpc_redis.h"
#include "message_target.h"
#include "share/define.h"
//#include "global_event.h"

#include <google/protobuf/message.h>

#include <list>
//#include <mutex>
#include <atomic>
#include <functional>

using namespace corpc;

namespace wukong {

    class MessageHandleManager
    {
        //typedef std::function<void (std::shared_ptr<MessageTarget>, uint16_t, std::shared_ptr<google::protobuf::Message>)> MessageHandle;

        struct RegisterMessageInfo {
            google::protobuf::Message *proto;
            MessageHandle handle;
            bool needCoroutine;
            bool needHotfix;
        };

        //struct HandleMessageInfo {
        //    std::shared_ptr<MessageTarget> obj;
        //    std::shared_ptr<google::protobuf::Message> msg;
        //    uint16_t tag;
        //    MessageHandle handle;
        //};
        //
        //struct HotfixMessageInfo {
        //    int msgType;
        //    std::shared_ptr<MessageTarget> obj;
        //    std::shared_ptr<google::protobuf::Message> msg;
        //    uint16_t tag;
        //};

    public:
        static MessageHandleManager& Instance() {
            static MessageHandleManager instance;
            return instance;
        }

        void init();
        
        bool registerMessage(int msgType,
                             google::protobuf::Message *proto,
                             bool needCoroutine,
                             MessageHandle handle);

        void handleMessage(std::shared_ptr<MessageTarget>, int msgType, uint16_t tag, const std::string &rawMsg);

        //GlobalEventListener& getGlobalEventListener() { return geventListener_; }

        bool getHotfixScript(int msgType, const char * &scriptBuf, size_t &bufSize);

    private:
        //static void *handleMessageRoutine(void * arg);

        //static void *hotfixMessageRoutine(void * arg);

        void resetHotfix();
        //static void callHotfix(std::shared_ptr<MessageTarget> obj, int msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg);

    private:
        std::map<int, RegisterMessageInfo> registerMessageMap_;
        std::map<int, std::string> hotfixMap_;

        //GlobalEventListener geventListener_; // 全服事件监听器（通过pubsub服务向redis订阅GEvent主题）

    private:
        MessageHandleManager() {}                                                      // ctor hidden
        ~MessageHandleManager() = default;                                             // destruct hidden
        MessageHandleManager(MessageHandleManager const&) = delete;                    // copy ctor delete
        MessageHandleManager(MessageHandleManager &&) = delete;                        // move ctor delete
        MessageHandleManager& operator=(MessageHandleManager const&) = delete;         // assign op. delete
        MessageHandleManager& operator=(MessageHandleManager &&) = delete;             // move assign op. delete
    };
}

#define g_MessageHandleManager wukong::MessageHandleManager::Instance()

#endif /* wukong_message_handle_manager_h */

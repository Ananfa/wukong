/*
 * Created by Xianke Liu on 2024/7/5.
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

#ifndef wukong_nexus_handler_h
#define wukong_nexus_handler_h

#include "corpc_message_terminal.h"

namespace wukong {
    class NexusHandler {
    public:
        static void registerMessages(corpc::MessageTerminal *terminal);

    private:
        static void connectHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);
        static void closeHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);
        
        static void accessHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);
        static void updateHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);

    private:
        NexusHandler() = default;                                 // ctor hidden
        NexusHandler(NexusHandler const&) = delete;               // copy ctor hidden
        NexusHandler(NexusHandler &&) = delete;                   // move ctor hidden
        NexusHandler& operator=(NexusHandler const&) = delete;    // assign op. hidden
        NexusHandler& operator=(NexusHandler &&) = delete;        // move assign op. hidden
        ~NexusHandler() = default;                                // dtor hidden
    };
}

#endif /* wukong_nexus_handler_h */

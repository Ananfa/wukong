/*
 * Created by Xianke Liu on 2020/12/22.
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

#ifndef wukong_gateway_handler_h
#define wukong_gateway_handler_h

#include "gateway_object_manager.h"
#include "corpc_message_server.h"

namespace wukong {
    class GatewayHandler {
    public:
        GatewayHandler(GatewayObjectManager *manager): manager_(manager) {}
        ~GatewayHandler() {}

        void registerMessages(corpc::TcpMessageServer *server);

    private:
        void connectHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn);
        void closeHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn);
        void banHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn);
        void authHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageServer::Connection> conn);

        void bypassHandle(int16_t type, uint16_t tag, std::shared_ptr<std::string> rawMsg, std::shared_ptr<corpc::MessageServer::Connection> conn);
    private:
        GatewayObjectManager *manager_;
    };
}

#endif /* wukong_gateway_handler_h */

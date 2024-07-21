/*
 * Created by Xianke Liu on 2024/7/11.
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

#ifndef wukong_gateway_agent_h
#define wukong_gateway_agent_h

#include "agent.h"
#include "share/const.h"
#include "gateway_service.pb.h"

namespace wukong {
    class GatewayAgent: public Agent {
    public:
        GatewayAgent(RpcClient *client): Agent(SERVER_TYPE_GATE, client) {}
        virtual ~GatewayAgent() {}

        virtual void addStub(ServerId sid, const std::string &host, int32_t port) override;

        void shutdown() override;
        bool kick(ServerId sid, UserId userId, const std::string &gToken);
        void broadcast(ServerId sid, int32_t type, uint16_t tag, const std::vector<std::pair<UserId, std::string>> &targets, const std::string &rawMsg);

    };
}

#endif /* wukong_gateway_agent_h */

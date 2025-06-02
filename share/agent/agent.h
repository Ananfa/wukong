/*
 * Created by Xianke Liu on 2024/7/16.
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

#ifndef wukong_agent_h
#define wukong_agent_h

#include "corpc_rpc_client.h"
#include "define.h"

#include <google/protobuf/service.h>

using namespace corpc;

namespace wukong {
    class Agent {
    public:
        struct StubInfo {
            pb::ServerInfo info;
            std::shared_ptr<google::protobuf::Service> stub;
        };

    public:
        Agent(ServerType serverType, RpcClient *client): serverType_(serverType), client_(client) {}
        virtual ~Agent() = 0;

        ServerType getType() { return serverType_; }

        void resetStubs(const std::list<pb::ServerInfo> &serverInfos);
        virtual void setStub(const pb::ServerInfo &serverInfo) = 0;
        void removeStub(ServerId sid);

        std::shared_ptr<google::protobuf::Service> getStub(ServerId sid);
        pb::ServerInfo* getServerInfo(ServerId sid);

        virtual void shutdown() = 0;

        virtual bool randomServer(ServerId &serverId); // 各类Agent可重载此方法

    protected:
        ServerType serverType_;
        RpcClient *client_;

        std::map<ServerId, StubInfo> stubInfos_;
    };

    class GameAgent: public Agent {
    public:
        GameAgent(ServerType serverType, RpcClient *client): Agent(serverType, client) {}
        virtual ~GameAgent() = 0;

        virtual void forwardIn(ServerId sid, int32_t type, uint16_t tag, RoleId roleId, std::shared_ptr<std::string> &rawMsg) = 0;
    };
}
    
#endif /* wukong_agent_h */

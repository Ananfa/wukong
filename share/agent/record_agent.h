/*
 * Created by Xianke Liu on 2024/7/19.
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

#ifndef wukong_record_agent_h
#define wukong_record_agent_h

#include "agent.h"
#include "share/const.h"
#include "record_service.pb.h"

namespace wukong {
    class RecordAgent: public Agent {
    public:
        RecordAgent(RpcClient *client): Agent(SERVER_TYPE_RECORD, client) {}
        virtual ~RecordAgent() {}

        virtual void addStub(ServerId sid, const std::string &host, int32_t port) override;

        void shutdown() override;

        bool loadRoleData(ServerId sid, RoleId roleId, UserId &userId, const std::string &lToken, ServerId &serverId, std::string &roleData); // 加载角色（游戏对象）

    };
}

#endif /* wukong_record_agent_h */
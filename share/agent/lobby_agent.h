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

#ifndef wukong_lobby_agent_h
#define wukong_lobby_agent_h

#include "agent.h"
#include "share/const.h"
#include "lobby_service.pb.h"

namespace wukong {
    class LobbyAgent: public GameAgent {
    public:
        LobbyAgent(RpcClient *client): GameAgent(SERVER_TYPE_LOBBY, client) {}
        virtual ~LobbyAgent() {}

        virtual void setStub(const pb::ServerInfo &serverInfo) override;

        void shutdown() override;
        virtual void forwardIn(ServerId sid, int32_t type, uint16_t tag, RoleId roleId, std::shared_ptr<std::string> &rawMsg) override;
        bool loadRole(ServerId sid, RoleId roleId, ServerId gwId); // 加载角色（游戏对象）
        void enterGame(ServerId sid, RoleId roleId, const std::string &lToken, ServerId gwId);

    };
}

#endif /* wukong_lobby_agent_h */

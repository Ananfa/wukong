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

#ifndef wukong_scene_agent_h
#define wukong_scene_agent_h

#include "agent.h"
#include "share/const.h"
#include "scene_service.pb.h"

namespace wukong {
    class SceneAgent: public GameAgent {
    public:
        SceneAgent(RpcClient *client): GameAgent(SERVER_TYPE_SCENE, client) {}
        virtual ~SceneAgent() {}

        virtual void setStub(const pb::ServerInfo &serverInfo) override;

        void shutdown() override;
        virtual void forwardIn(ServerId sid, int16_t type, uint16_t tag, RoleId roleId, std::shared_ptr<std::string> &rawMsg) override;
        
        std::string loadScene(ServerId sid, uint32_t defId, const std::string &sceneId, RoleId roleId, const std::string &teamId);
        void enterScene(ServerId sid, const std::string &sceneId, RoleId roleId, ServerId gwId);
        
    };
}

#endif /* wukong_scene_agent_h */

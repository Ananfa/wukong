/*
 * Created by Xianke Liu on 2022/2/23.
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

#ifndef wukong_scene_role_h
#define wukong_scene_role_h

using namespace corpc;

namespace wukong {

	class SceneRole: public MessageTarget {
	public:
		SceneRole(UserId userId, RoleId roleId, ServerId lobbyId, ServerId gatewayId): userId_(userId), roleId_(roleId), lobbyId_(lobbyId), gatewayId_(gatewayId) {}
        virtual ~SceneRole() {};

        virtual bool initData(const std::string &data) = 0;

    protected:
        UserId userId_;
        RoleId roleId_;
        ServerId lobbyId_;
        ServerId gatewayId_;
	};
}

#endif /* wukong_scene_role_h */

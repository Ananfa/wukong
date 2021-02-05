/*
 * Created by Xianke Liu on 2021/1/15.
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

#ifndef game_object_manager_h
#define game_object_manager_h

#include "corpc_message_server.h"
#include "corpc_redis.h"
#include "game_object.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    class GameObjectManager {
    public:
        GameObjectManager(ServerId id):_id(id), _shutdown(false) {}

        void init();

        ServerId getId() { return _id; }

        void shutdown();
        bool isShutdown() { return _shutdown; }

        size_t size(); // 获取当前游戏对象数

        bool exist(RoleId roleId); 
        std::shared_ptr<GameObject> getGameObject(RoleId roleId);
        std::shared_ptr<GameObject> create(UserId userId, RoleId roleId, uint32_t lToken, ServerId gatewayId, ServerId recordId, const std::string &data);
        bool remove(RoleId roleId); // 删除玩家游戏对象
    private:
        ServerId _id;       // lobby服务号
        bool _shutdown;

        // 游戏对象列表
        std::map<RoleId, std::shared_ptr<GameObject>> _roleId2GameObjectMap;
    };

}

#endif /* game_object_manager_h */

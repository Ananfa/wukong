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

#ifndef wukong_game_object_manager_h
#define wukong_game_object_manager_h

#include "corpc_message_server.h"
#include "game_object.h"
#include "global_event.h"
#include "share/define.h"
#include <functional>

using namespace corpc;

namespace wukong {
    class GameObjectManager {
    public:
        GameObjectManager(GameServerType type, ServerId id):type_(type), id_(id), shutdown_(false) {}
        virtual ~GameObjectManager() {}

        void init();

        ServerId getId() { return id_; }

        virtual void shutdown();
        bool isShutdown() { return shutdown_; }

        size_t roleCount(); // 获取当前游戏对象数

        bool existRole(RoleId roleId); 
        std::shared_ptr<GameObject> getGameObject(RoleId roleId);
        bool loadRole(RoleId roleId, ServerId gatewayId);
        virtual void leaveGame(RoleId roleId); // 离开游戏--删除玩家游戏对象（只在心跳失败时调用，离开场景（离队）不调用此方法）
        //virtual void offline(RoleId roleId); // 玩家离线

        // 全服事件相关接口
        uint32_t regGlobalEventHandle(const std::string &name, EventHandle handle);
        void unregGlobalEventHandle(uint32_t refId);
        void fireGlobalEvent(const Event &event); // 注意：全局事件中只能放一个叫"data"的std::string类型数据

        // TODO: 实现广播和多播接口

    protected:
        GameServerType type_; // 游戏服务器类型（大厅、场景...）
        ServerId id_;         // 游戏服务器号（大厅服号、场景服号...）
        bool shutdown_;

        // 游戏对象列表
        std::map<RoleId, std::shared_ptr<GameObject>> roleId2GameObjectMap_;

        // 全服事件相关
        GlobalEventDispatcher geventDispatcher_;
    };

}

#endif /* wukong_game_object_manager_h */

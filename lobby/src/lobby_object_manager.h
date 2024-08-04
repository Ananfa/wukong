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

#ifndef wukong_lobby_object_manager_h
#define wukong_lobby_object_manager_h

#include "corpc_message_server.h"
#include "lobby_object.h"
#include "global_event.h"
#include "share/define.h"
#include <functional>

using namespace corpc;

namespace wukong {
    class LobbyObjectManager {
    public:
        static LobbyObjectManager& Instance() {
            static LobbyObjectManager theSingleton;
            return theSingleton;
        }

    public:
        void init();

        virtual void shutdown();
        bool isShutdown() { return shutdown_; }

        bool existRole(RoleId roleId); 
        std::shared_ptr<LobbyObject> getLobbyObject(RoleId roleId);
        bool loadRole(RoleId roleId, ServerId gatewayId);
        virtual void leaveGame(RoleId roleId); // 离开游戏--删除玩家游戏对象（只在心跳失败时调用，离开场景（离队）不调用此方法）

        // TODO: 实现广播和多播接口

    protected:
        bool shutdown_;

        // 游戏对象列表
        std::map<RoleId, std::shared_ptr<LobbyObject>> roleId2LobbyObjectMap_;

    private:
        LobbyObjectManager(): shutdown_(false) {}                             // ctor hidden
        LobbyObjectManager(LobbyObjectManager const&) = delete;               // copy ctor hidden
        LobbyObjectManager(LobbyObjectManager &&) = delete;                   // move ctor hidden
        LobbyObjectManager& operator=(LobbyObjectManager const&) = delete;    // assign op. hidden
        LobbyObjectManager& operator=(LobbyObjectManager &&) = delete;        // move assign op. hidden
        ~LobbyObjectManager() = default;                                      // dtor hidden
    };

}

#define g_LobbyObjectManager wukong::LobbyObjectManager::Instance()

#endif /* wukong_lobby_object_manager_h */

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
#include "game_object.h"
#include "event.h"
#include "share/define.h"
#include <functional>

using namespace corpc;

namespace wukong {
    typedef std::function<void (const std::string &topic, const std::string &data)> GlobalEventHandle;

    class GameObjectManager {
    public:
        GameObjectManager(GameServerType type, ServerId id):_type(type), _id(id), _shutdown(false) { _eventQueue = std::make_shared<GlobalEventQueue>(); }
        virtual ~GameObjectManager() {}

        void init();

        ServerId getId() { return _id; }

        void shutdown();
        bool isShutdown() { return _shutdown; }

        size_t roleCount(); // 获取当前游戏对象数

        bool existRole(RoleId roleId); 
        std::shared_ptr<GameObject> getGameObject(RoleId roleId);
        bool loadRole(RoleId roleId, ServerId gatewayId);
        void leaveGame(RoleId roleId); // 离开游戏--删除玩家游戏对象（只在心跳失败时调用，离开场景（离队）不调用此方法）

        // 全服事件相关接口
        uint32_t regGlobalEventHandle(const std::string &name, EventHandle handle);
        void unregGlobalEventHandle(uint32_t refId);
        void fireGlobalEvent(const Event &event); // 注意：全局事件中只能放一个叫"data"的std::string类型数据
        void fireGlobalEvent(const std::string &topic, const std::string &data);

        // TODO: 实现广播和多播接口

    private:
        static void *globalEventHandleRoutine(void * arg);

    private:
        GameServerType _type; // 游戏服务器类型（大厅、场景...）
        ServerId _id;         // 游戏服务器号（大厅服号、场景服号...）
        bool _shutdown;

        // 游戏对象列表
        std::map<RoleId, std::shared_ptr<GameObject>> _roleId2GameObjectMap;

        // 全服事件相关
        std::shared_ptr<GlobalEventQueue> _eventQueue;
        EventEmitter _emiter;
    };

}

#endif /* game_object_manager_h */

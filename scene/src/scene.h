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

#ifndef wukong_scene_h
#define wukong_scene_h

#include "corpc_cond.h"
#include "share/define.h"
#include "game_object.h"
#include "event.h"
#include <map>

using namespace corpc;

namespace wukong {
    class Scene;
    class SceneManager;

    struct SceneRoutineArg {
        std::shared_ptr<Scene> obj;
    };

    class Scene: public std::enable_shared_from_this<Scene> {
    public:
        Scene(uint32_t defId, SceneType type, const std::string &sceneId, const std::string &sToken, SceneManager *manager): defId_(defId), type_(type), sceneId_(sceneId), sToken_(sToken), manager_(manager) {}
        virtual ~Scene() {};

        uint32_t getDefId() { return defId_; }
        const std::string &getSceneId() { return sceneId_; }
        const std::string &getSToken() { return sToken_; }

        SceneType getType() { return type_; }

        void start(); // 开始心跳，启动心跳协程（个人场景不需要redis心跳，gameobj有心跳就够了，gameobj销毁时scene也要销毁，由scene的update进行判断--场景没人时自毁处理）
        void stop(); // 停止心跳，清除游戏对象列表（调用游戏对象stop）
        
        virtual void update(timeval now) = 0; // 周期处理逻辑（注意：不要有产生协程切换的逻辑）
        virtual void onEnter(RoleId roleId) = 0; // 如：可以进行进入场景AOI通知
        virtual void onLeave(RoleId roleId) = 0; // 如：可以进行离开场景AOI通知
        virtual void onDestory() = 0;

        bool enter(std::shared_ptr<GameObject> role);
        bool leave(RoleId roleId);
        
        // 注意：这里不提供注销事件处理的接口方法，在gameobject销毁（stop）的时候一次性注销所有绑定的事件处理
        void regLocalEventHandle(const std::string &name, EventHandle handle);
        void regGlobalEventHandle(const std::string &name, EventHandle handle);
        
        void fireLocalEvent(const Event &event);
        void fireGlobalEvent(const Event &event);

    private:
        static void *heartbeatRoutine(void *arg);  // 非个人场景需要启动心跳协程，周期对scenelocation重设超时时间，心跳失败时需通知Manager销毁场景对象

        static void *updateRoutine(void *arg); // 逻辑协程（周期逻辑更新）

    private:
        std::string sToken_;

        SceneType type_;

        bool running_ = false;

        Cond cond_;

        EventEmitter emiter_;
        std::vector<uint32_t> globalEventHandleRefs_; // 用于gameobject销毁时注销注册的全局事件处理

    protected:
        uint32_t defId_;
        std::string sceneId_;

        SceneManager *manager_; // 关联的场景manager

        // 场景中的游戏对象列表（注意：不要产生shared_ptr循环引用）
        std::map<RoleId, std::shared_ptr<GameObject>> roles_;

    public:
        friend class SceneManager;
        friend class InnerSceneServiceImpl;
    };
}

#endif /* wukong_scene_h */

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

#ifndef scene_h
#define scene_h

#include <map>

using namespace corpc;

namespace wukong {
    struct SceneRoutineArg {
        std::shared_ptr<Scene> obj;
    };

    class Scene: public std::enable_shared_from_this<Scene> {
    public:
        Scene(uint32_t defId, SceneType type, const std::string &sceneId, const std::string &sToken, SceneManager *manager): _defId(defId), _type(type), _sceneId(sceneId), _sToken(sToken), _manager(manager) {}
        virtual ~Scene() = 0;

        uint32_t getDefId() { return _defId; }
        const std::string &getSceneId() { return _sceneId; }
        uint32_t getSToken() { return _sToken; }

        SceneType getType() { return _type; }

        void start(); // 开始心跳，启动心跳协程（个人场景不需要redis心跳，gameobj有心跳就够了，gameobj销毁时scene也要销毁，由scene的update进行判断--场景没人时自毁处理）
        void stop(); // 停止心跳，清除游戏对象列表（调用游戏对象stop）
        
        virtual void update(timeval now) = 0; // 周期处理逻辑（注意：不要有产生协程切换的逻辑）

        bool enter(std::shared_ptr<GameObject> role);
        bool leave(RoleId roleId);
        
        void onEnter(RoleId roleId) = 0; // 如：可以进行进入场景AOI通知
        void onLeave(RoleId roleId) = 0; // 如：可以进行离开场景AOI通知

    private:
        static void *heartbeatRoutine(void *arg);  // 非个人场景需要启动心跳协程，周期对scenelocation重设超时时间，心跳失败时需通知Manager销毁场景对象

        static void *updateRoutine(void *arg); // 逻辑协程（周期逻辑更新）

    private:
        std::string _sToken;

        SceneType _type;

        bool _running = false;

        Cond _cond;

    	SceneManager *_manager; // 关联的场景manager

    	// 场景中的游戏对象列表（注意：不要产生shared_ptr循环引用）
    	std::map<RoleId, std::shared_ptr<GameObject>> _roles;

    protected:
        uint32_t _defId;
        std::string _sceneId;

    public:
        friend class SceneManager;
	};
}

#endif /* scene_h */

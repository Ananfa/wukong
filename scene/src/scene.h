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

    class Scene: public std::enable_shared_from_this<Scene> {
    public:
        Scene(uint32_t defId, uint32_t sceneId, uint32_t sToken, SceneManager *manager): _defId(defId), _sceneId(sceneId), _sToken(sToken), _manager(manager) {}
        virtual ~Scene() = 0;

        // 问题：场景数据加载在scene manager进行，但是场景数据每个游戏都不同，怎样定义初始化场景接口？通过pb的编解码？通过pb基类指针？通过void *指针转换
        virtual bool initData(void *data) = 0;

        uint32_t getDefId() { return _defId; }
        uint32_t getSceneId() { return _sceneId; }
        uint32_t getSToken() { return _sToken; }

        void start(); // 开始心跳，启动心跳协程（个人场景不需要redis心跳，gameobj有心跳就够了，gameobj销毁时scene也要销毁，由scene的update进行判断--场景没人时自毁处理）
        void stop(); // 停止心跳，清除游戏对象列表（调用游戏对象stop）
        // 问题：游戏对象心跳失败销毁时如何通知scene清除游戏对象？scene在update中判断游戏对象状态，清理running为false的游戏对象（解耦，避免游戏对象stop时直接调用scene的游戏对象列表）

        virtual void update(uint64_t nowSec) = 0; // 周期处理逻辑（逻辑包括清理running为false的游戏对象）

        void addRole(std::shared_ptr<GameObject> role);
        //void removeRole(RoleId roleId); // 好像不需要这个接口，现在的场景切换流程是对象销毁重建（会从一个场景直接将对象挪到另一个场景吗？），对象销毁后由update逻辑清理

    private:
        uint32_t _sToken;

        // 是否多人场景？单人场景不进行redis心跳，单人场景在场景加载时会同时加载gameObj
        bool _multiRolesScene;
        // 是否永久（非副本）场景？副本场景没人时会自毁
        bool _forever;

    	SceneManager *_manager; // 关联的场景manager

    	// 场景中的游戏对象列表（注意：不要产生shared_ptr循环引用）
    	std::map<RoleId, std::shared_ptr<GameObject>> _roles;

    protected:
        uint32_t _defId;
        uint32_t _sceneId;
	};
}

#endif /* scene_h */

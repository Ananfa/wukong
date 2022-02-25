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

        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳，清除游戏对象列表（调用游戏对象stop）

        virtual void update(uint64_t nowSec) = 0; // 周期处理逻辑

        // TODO: 在scene中包含game_object_manager，角色游戏对象需要在场景中加载

    private:
        uint32_t _sToken;

    	SceneManager *_manager; // 关联的场景manager

    	// 场景中的游戏对象列表
    	std::map<RoleId, std::shared_ptr<GameObject>> _roles;

    protected:
        uint32_t _defId;
        uint32_t _sceneId;
	};
}

#endif /* scene_h */

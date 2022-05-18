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

#ifndef wukong_scene_delegate_h
#define wukong_scene_delegate_h

#include <functional>
#include "scene.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    class SceneDelegate {
    	typedef std::function<std::shared_ptr<Scene> (uint32_t defId, SceneType sType, const std::string& sceneId, const std::string &stoken, SceneManager *manager)> CreateSceneHandle; // 参数1.场景配置ID，参数2.场景实例ID（为0表示需要产生实例ID），返回.场景实例shared_ptr对象
        typedef std::function<std::list<RoleId> (const std::string&)> GetMembersHandle; // 参数为队伍ID，返回队伍成员ID列表（乌拉拉类型游戏）
        typedef std::function<SceneType (uint32_t)> GetSceneTypeHandle; // 参数为场景配置ID

    public:
        static SceneDelegate& Instance() {
            static SceneDelegate instance;
            return instance;
        }

        void setCreateSceneHandle(CreateSceneHandle handle) { _createScene = handle; }
        CreateSceneHandle getCreateSceneHandle() { return _createScene; }

        void setGetMembersHandle(GetMembersHandle handle) { _getMembers = handle; }
        GetMembersHandle getGetMembersHandle() { return _getMembers; }

        void setGetSceneTypeHandle(GetSceneTypeHandle handle) { _getSceneType = handle; }
        GetSceneTypeHandle getGetSceneTypeHandle() { return _getSceneType; }

    private:
        CreateSceneHandle _createScene;
        GetMembersHandle _getMembers;
        GetSceneTypeHandle _getSceneType;
    };
}

#define g_SceneDelegate SceneDelegate::Instance()

#endif /* wukong_scene_delegate_h */

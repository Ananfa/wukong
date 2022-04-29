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

#ifndef lobby_delegate_h
#define lobby_delegate_h

#include <functional>
#include "share/define.h"

using namespace corpc;

namespace wukong {
    class LobbyDelegate {
        typedef std::function<std::string (RoleId)> GetTargetSceneIdHandle; // 获取登录时角色应进入的场景实例号，返回0表示在大厅登录
        typedef std::function<bool (uint32_t)> NeedLoadSceneHandle; // 判断场景（参数为场景实例号）不存在时是否需要加载场景
        typedef std::function<bool (uint32_t)> IsSceneAutoLoadRoleHandle; // 判断场景（参数为场景实例号）是否会自动加载角色

    public:
        static LobbyDelegate& Instance() {
            static LobbyDelegate instance;
            return instance;
        }

        void setGetTargetSceneIdHandle(GetTargetSceneIdHandle handle) { _getTargetSceneId = handle; }
        GetTargetSceneIdHandle getGetTargetSceneIdHandle() { return _getTargetSceneId; }

        void setNeedLoadSceneHandle(NeedLoadSceneHandle handle) { _needLoadScene = handle; }
        NeedLoadSceneHandle getNeedLoadSceneHandle() { return _needLoadScene; }

        void setIsSceneAutoLoadRoleHandle(IsSceneAutoLoadRoleHandle handle) { _isSceneAutoLoadRole = handle; }
        IsSceneAutoLoadRoleHandle getIsSceneAutoLoadRoleHandle() { return _isSceneAutoLoadRole; }

    private:
        GetTargetSceneIdHandle _getTargetSceneId;
        // 注意：_needLoadScene和_isSceneAutoLoadRole只有当会登录到场景中时才需要设置
        NeedLoadSceneHandle _needLoadScene;
        IsSceneAutoLoadRoleHandle _isSceneAutoLoadRole;
    };
}

#define g_LobbyDelegate LobbyDelegate::Instance()

#endif /* lobby_delegate_h */

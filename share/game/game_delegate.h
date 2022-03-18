/*
 * Created by Xianke Liu on 2021/6/10.
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

#ifndef game_delegate_h
#define game_delegate_h

#include <functional>
#include "game_object_manager.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    class GameDelegate {
        // 注意：CreateGameObjectHandle中不应有产生协程切换的实现
        typedef std::function<std::shared_ptr<GameObject> (UserId, RoleId, ServerId, const std::string &ltoken, GameObjectManager*, const std::string &data)> CreateGameObjectHandle;

    public:
        static GameDelegate& Instance() {
            static GameDelegate instance;
            return instance;
        }

        void setCreateGameObjectHandle(CreateGameObjectHandle handle) { _createGameObject = handle; }
        CreateGameObjectHandle getCreateGameObjectHandle() { return _createGameObject; }

    private:
        CreateGameObjectHandle _createGameObject;

    private:
        GameDelegate() = default;                                 // ctor hidden
        GameDelegate(GameDelegate const&) = delete;               // copy ctor hidden
        GameDelegate(GameDelegate &&) = delete;                   // move ctor hidden
        GameDelegate& operator=(GameDelegate const&) = delete;    // assign op. hidden
        GameDelegate& operator=(GameDelegate &&) = delete;        // move assign op. hidden
        ~GameDelegate() = default;                                // dtor hidden
    };
}

#define g_GameDelegate GameDelegate::Instance()

#endif /* game_delegate_h */

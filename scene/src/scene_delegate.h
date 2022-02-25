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

#ifndef scene_delegate_h
#define scene_delegate_h

#include <functional>
#include "record_object_manager.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    class SceneDelegate {
    	typedef std::function<uint32_t (uint32_t, uint32_t)> LoadSceneHandle; // 参数1.场景配置ID，参数2.场景实例ID（为0表示需要产生实例ID），返回.场景实例shared_ptr对象

    public:
        static SceneDelegate& Instance() {
            static SceneDelegate instance;
            return instance;
        }

        void setLoadSceneHandle(LoadSceneHandle handle) { _loadScene = handle; }
        LoadSceneHandle getLoadSceneHandle() { return _loadScene; }

    private:
        LoadSceneHandle _loadScene;
    };
}

#define g_SceneDelegate SceneDelegate::Instance()

#endif /* scene_delegate_h */

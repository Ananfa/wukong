/*
 * Created by Xianke Liu on 2022/2/24.
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

#ifndef scene_manager_h
#define scene_manager_h

using namespace corpc;

namespace wukong {
    class SceneManager {
    public:
        SceneManager(ServerId id):_id(id), _shutdown(false) {}

        void init();

        ServerId getId() { return _id; }

        void shutdown();
        bool isShutdown() { return _shutdown; }

        size_t size(); // 获取当前场景对象数

        bool exist(uint32_t sceneId); 
        std::shared_ptr<Scene> getScene(uint32_t sceneId);
        uint32_t loadScene(uint32_t defId, uint32_t sceneId);
        bool remove(uint32_t sceneId); // 删除场景对象

    private:
        ServerId _id;         // 场景服务器号
        bool _shutdown;

        // 场景列表
        std::map<uint32_t, std::shared_ptr<Scene>> _sceneId2SceneMap;
    };

}

#endif /* scene_manager_h */

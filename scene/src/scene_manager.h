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

#include <list>
#include <map>
#include <string>

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

        bool exist(const std::string &sceneId); 
        std::shared_ptr<Scene> getScene(const std::string &sceneId);
        std::string loadScene(uint32_t defId, const std::string &sceneId, RoleId roleid, const std::string &teamid); // 失败时返回0
        bool remove(const std::string &sceneId); // 删除场景对象

    private:
        ServerId _id;         // 场景服务器号
        bool _shutdown;

        uint64_t _incSceneNo = 0; // 场景自增计数

        // 场景列表
        std::map<std::string, std::shared_ptr<Scene>> _sceneId2SceneMap;
    };

}

#endif /* scene_manager_h */

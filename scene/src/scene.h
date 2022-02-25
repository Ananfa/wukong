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

using namespace corpc;

namespace wukong {

    class Scene: public std::enable_shared_from_this<Scene> {
    public:
        Scene(uint32_t defId, uint32_t sceneId, uint32_t sToken, SceneManager *manager): _defId(defId), _sceneId(sceneId), _sToken(sToken), _manager(manager) {}
        virtual ~Scene() = 0;

        // 问题：场景数据加载在scene manager进行，但是场景数据每个游戏都不同，怎样定义初始化场景接口？通过pb的编解码？通过pb基类指针？通过void *指针转换
        virtual bool initData(void *data) = 0;

        
	};
}

#endif /* scene_h */

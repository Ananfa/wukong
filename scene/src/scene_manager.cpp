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

#include "corpc_routine_env.h"
#include "scene_manager.h"
#include "scene_delegate.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

void SceneManager::shutdown() {
    if (_shutdown) {
        return;
    }

    _shutdown = true;

    for (auto &scene : _sceneId2SceneMap) {
        scene.second->stop();
    }

    _sceneId2SceneMap.clear();

    for (auto &gameObj : _roleId2GameObjectMap) {
        gameObj.second->stop();
    }

    _roleId2GameObjectMap.clear();
}

size_t SceneManager::sceneCount() {
    return _sceneId2SceneMap.size();
}

bool SceneManager::existScene(const std::string &sceneId) {
    return _sceneId2SceneMap.find(sceneId) != _sceneId2SceneMap.end();
}

std::shared_ptr<Scene> SceneManager::getScene(const std::string &sceneId) {
    auto it = _sceneId2SceneMap.find(sceneId);
    if (it == _sceneId2SceneMap.end()) {
        return nullptr;
    }

    return it->second;
}

std::string SceneManager::loadScene(uint32_t defId, const std::string &sceneId, RoleId roleid, const std::string &teamid) {
    if (!sceneId.empty() && _sceneManager->exist(sceneId)) {
        ERROR_LOG("SceneManager::loadScene -- scene %s already exist\n", sceneId.c_str());
        return "";
    }

    // 通过delegate中的getSceneType来获取场景类型：单人副本、多人副本、永久场景
    // 问题：对于永久场景，可能会有负载均衡分线需求，此时永久场景资源的动态伸缩如何进行？通过人工干预？该问题是游戏上层解决，不在框架考虑范围
    assert(g_SceneDelegate.getGetSceneTypeHandle());
    SceneType sType = g_SceneDelegate.getGetSceneTypeHandle()(defId);

    if (sType == SCENE_TYPE_SINGLE_PLAYER && roleid == 0) {
        ERROR_LOG("SceneManager::loadScene -- single player scene should have owner roleid\n");
        return "";
    }

    if (sType == SCENE_TYPE_TEAM && teamid.empty()) {
        ERROR_LOG("SceneManager::loadScene -- team scene should have teamid\n");
        return "";
    }

    // 根据规则生成场景实例号
    // 场景号生成规则：
    // 个人副本场景实例号：PS_<roleID>
    // 多人副本场景实例号：MS_<场景服ID>_<场景服本地自增计数>
    // W3L队伍场景实例号：TS_<队伍号>，由加载场景接口调用时传入，否则会按照多人副本场景实例号规则
    // 世界地图场景实例号：GS_<场景定义ID>
    std::string realSceneId;
    if (sceneId.empty()) {
        switch (sType) {
            case SCENE_TYPE_SINGLE_PLAYER: {
                realSceneId = "PS_" + std::to_string(members.front());
                break;
            }
            case SCENE_TYPE_MULTI_PLAYER: {
                realSceneId = "MS_" + std::to_string(_id) + "_" + std::to_string(_incSceneNo);
                break;
            }
            case SCENE_TYPE_TEAM: {
                realSceneId = "TS_" + teamid;
            }
            case SCENE_TYPE_GLOBAL: {
                realSceneId = "GS_" + std::to_string(defId);
                break;
            }
        }
    }

    // 如果不是个人场景，需要向redis进行注册新场景地址
    if (sType != SCENE_TYPE_SINGLE_PLAYER) {
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("SceneManager::loadScene -- connect to cache failed\n");
            return "";
        }

        // 生成lToken（直接用当前时间来生成）
        struct timeval t;
        gettimeofday(&t, NULL);
        std::string sToken = std::to_string((t.tv_sec % 1000) * 1000000 + t.tv_usec);

        switch (RedisUtils::SetSceneAddress(cache, realSceneId, sToken, _id)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("SceneManager::loadScene -- set scene:%s location failed\n", realSceneId.c_str());
                return "";
            }
            case REDIS_FAIL: {
                freeReplyObject(reply);
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                ERROR_LOG("SceneManager::loadScene -- set scene:%s location failed for already set\n", realSceneId.c_str());
                return "";
            }
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
    }

    // 应该在这里(上了锁之后)获取成员列表
    // 获取场景需预载入的玩家ID列表（对于单人场景就算场景拥有者玩家ID，对于W3L类型队伍场景就是队伍成员列表）
    //       对于个人场景（预加载玩家ID为request参数中的roleid）
    //       对于非个人场景（通过delegate中的GetMembers接口获得预加载列表）
    std::list<uint32_t> members;
    if (roleid != 0) {
        members.push_back(roleid);
    } else if (!teamid.empty()) {
        assert(g_SceneDelegate.setGetMembersHandle());
        members = g_SceneDelegate.setGetMembersHandle()(teamid); // 这里会产生协程切换，若操作太慢会导致锁被释放
    }

    // 创建场景对象
    assert(g_SceneDelegate.getCreateSceneHandle());
    auto scene = g_SceneDelegate.getCreateSceneHandle()(defId, sType, realSceneId);

    if (!scene) {
        ERROR_LOG("SceneManager::loadScene -- load scene failed\n");
        return "";
    }

    // 启动场景对象
    _sceneId2SceneMap.insert(std::make_pair(realSceneId, scene));
    scene->start();

    // 成员预加载，若其中有玩家gameObj预加载失败，将所有已预加载的对象销毁并且销毁场景对象，返回加载失败
    for (auto roleId : members) {
        if (_gameObjectManager->loadRole(roleId, 0)) {
            scene->enter(_roleId2GameObjectMap[roleId]);
        } else {
            ERROR_LOG("SceneManager::loadScene -- load scene defId[%d] sceneId[%s] failed for load role[%llu] failed\n", defId, realSceneId.c_str(), roleId);
            
            removeScene(realSceneId);
            return "";
        }
    }

    return realSceneId;
}

void SceneManager::removeScene(const std::string &sceneId) {
    auto it = _sceneId2SceneMap.find(sceneId);
    assert(it != _sceneId2SceneMap.end());

    // 踢出场景中所有玩家(强制离线)，正常的离线不是由场景销毁导致的
    for (auto it1 : it->second->_roles) {
        DEBUG_LOG("SceneManager::removeScene -- remove role %d\n", it1->first);
        // 删除游戏对象
        it1->second->stop();
        _roleId2GameObjectMap.erase(it1->first);
    }

    it->second->stop();
    _sceneId2SceneMap.erase(it);
}

void SceneManager::leaveGame(RoleId roleId) {
    // 如果角色与场景绑定，需要一并销毁场景（如：W3L队伍场景（离队不算），个人场景）
    auto it = _roleId2GameObjectMap.find(roleId);
    assert(it != _roleId2GameObjectMap.end());

    const std::string &sceneId = it->second->getSceneId();
    if (!sceneId.empty()) {
        auto it1 = _sceneId2SceneMap.find(sceneId);
        assert(it1 != _sceneId2SceneMap.end());  // 不应该找不到场景

        SceneType sType = it1->second->getType();
        if (sType == SCENE_TYPE_SINGLE_PLAYER || sType == SCENE_TYPE_TEAM) {
            // 直接销毁场景（由于场景绑定角色或队伍，少一个人都不行）
            removeScene(sceneId);
        } else {
            // 按离开场景逻辑走
            leaveScene(roleId);

            // 删除游戏对象
            it->second->stop();
            _roleId2GameObjectMap.erase(it);
        }
    } else {
        // 删除游戏对象
        it->second->stop();
        _roleId2GameObjectMap.erase(it);
    }
}

void SceneManager::leaveScene(RoleId roleId) {
    auto it = _roleId2GameObjectMap.find(roleId);
    assert(it != _roleId2GameObjectMap.end());

    const std::string &sceneId = it->second->getSceneId();
    auto it1 = _sceneId2SceneMap.find(sceneId);
    assert(it1 != _sceneId2SceneMap.end());  // 不应该找不到场景

    it1->second->leave(roleId);

    return true;
}
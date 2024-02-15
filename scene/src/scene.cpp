/*
 * Created by Xianke Liu on 2022/3/31.
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
#include "scene.h"
#include "scene_config.h"
#include "scene_manager.h"
#include "redis_pool.h"
#include "redis_utils.h"

using namespace corpc;
using namespace wukong;

void Scene::start() {
    running_ = true;

    // 启动心跳协程
    if (type_ != SCENE_TYPE_SINGLE_PLAYER) {
        SceneRoutineArg *arg = new SceneRoutineArg();
        arg->obj = shared_from_this();
        RoutineEnvironment::startCoroutine(heartbeatRoutine, arg);
    }

    // 启动update协程
    if (g_SceneConfig.getUpdatePeriod() > 0) {
        SceneRoutineArg *arg = new SceneRoutineArg();
        arg->obj = shared_from_this();
        RoutineEnvironment::startCoroutine(updateRoutine, arg);
    }
}

void Scene::stop() {
    if (running_) {
        running_ = false;

        // 若不清emiter，会导致shared_ptr循环引用问题
        emiter_.clear();

        for (auto ref : globalEventHandleRefs_) {
            manager_->unregGlobalEventHandle(ref);
        }
        globalEventHandleRefs_.clear();

        roles_.clear();

        onDestory();

        cond_.broadcast();

        // 非个人场景删除scene location key
        if (type_ != SCENE_TYPE_SINGLE_PLAYER) {
            redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
            if (!cache) {
                ERROR_LOG("Scene::stop -- connect to cache failed when remove scene address\n");
                return;
            }
            // 释放SceneLocation
            if (RedisUtils::RemoveSceneAddress(cache, sceneId_, sToken_) == REDIS_DB_ERROR) {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("Scene::stop -- remove scene:%s location failed\n", sceneId_.c_str());
            } else {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
            }
        }
    }
}

bool Scene::enter(std::shared_ptr<GameObject> role) {
    if (!running_) {
        return false;
    }

    roles_.insert(std::make_pair(role->getRoleId(), role));
    role->setSceneId(sceneId_);

    onEnter(role->getRoleId());
    return true;
}

bool Scene::leave(RoleId roleId) {
    auto it = roles_.find(roleId);
    if (it == roles_.end()) {
        return false;
    }

    onLeave(roleId);

    // 注意：如果onLeave中会进行协程切换，这里可能会已经被删除了（比如：场景进入了销毁流程），因此这里不用erase(it)而是用erase(roleId)
    roles_.erase(roleId);
    return true;
}

void Scene::regLocalEventHandle(const std::string &name, EventHandle handle) {
    emiter_.addEventHandle(name, handle);
}

void Scene::regGlobalEventHandle(const std::string &name, EventHandle handle) {
    uint32_t ref = manager_->regGlobalEventHandle(name, handle);
    globalEventHandleRefs_.push_back(ref);
}

void Scene::fireLocalEvent(const Event &event) {
    emiter_.fireEvent(event);
}

void Scene::fireGlobalEvent(const Event &event) {
    manager_->fireGlobalEvent(event);
}

void *Scene::heartbeatRoutine(void *arg) {
    SceneRoutineArg* routineArg = (SceneRoutineArg*)arg;
    std::shared_ptr<Scene> obj = std::move(routineArg->obj);
    delete routineArg;

    int failTimes = 0;
    while (obj->running_) {
        // 目前只有心跳和停服会销毁游戏对象
        obj->cond_.wait(TOKEN_HEARTBEAT_PERIOD);

        if (!obj->running_) {
            // 场景已被销毁
            break;
        }

        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] connect to cache failed\n", obj->sceneId_.c_str(), obj->defId_);

            failTimes++;
        } else {
            switch (RedisUtils::SetSceneAddressTTL(cache, obj->sceneId_, obj->sToken_)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCoreCache()->put(cache, true);
                    ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] check token failed for db error\n", obj->sceneId_.c_str(), obj->defId_);
                    failTimes++;
                    break;
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] token[%s] check session failed\n", obj->sceneId_.c_str(), obj->defId_, obj->sToken_.c_str());
                    failTimes += 3; // 直接失败
                    break;
                }
                case REDIS_SUCCESS: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    failTimes = 0;
                    break;
                }
            }
        }

        // 若超过3次设置失败，销毁场景对象
        if (failTimes >= 3) {
            if (obj->running_) {
                ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] heartbeat failed and destroyed\n", obj->sceneId_.c_str(), obj->defId_);
                obj->manager_->removeScene(obj->sceneId_);
                assert(obj->running_ == false);
            }
        }

        // TODO: 无人副本场景销毁
    }

    return nullptr;
}

void *Scene::updateRoutine(void *arg) {
    SceneRoutineArg* routineArg = (SceneRoutineArg*)arg;
    std::shared_ptr<Scene> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;

    while (obj->running_) {
        obj->cond_.wait(g_SceneConfig.getUpdatePeriod());

        if (!obj->running_) {
            // 场景已被销毁
            break;
        }

        gettimeofday(&t, NULL);
        obj->update(t);
    }

    return nullptr;
}
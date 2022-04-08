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

void Scene::start() {
    _running = true;

    // 启动心跳协程
    if (_type != SCENE_TYPE_SINGLE_PLAYER) {
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
    if (_running) {
        DEBUG_LOG("Scene::stop sceneId[%s] defId[%d]\n", _sceneId.c_str(), _defId);
        _running = false;

        // 若不清emiter，会导致shared_ptr循环引用问题
        _emiter.clear();

        for (auto ref : _globalEventHandleRefs) {
            _manager->unregGlobalEventHandle(ref);
        }
        _globalEventHandleRefs.clear();

        _roles.clear();

        onDestory();

        _cond.broadcast();

        // 非个人场景删除scene location key
        if (_type != SCENE_TYPE_SINGLE_PLAYER) {
            redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
            if (!cache) {
                ERROR_LOG("Scene::stop -- connect to cache failed when remove scene address\n");
                return;
            }
            // 释放SceneLocation
            if (RedisUtils::RemoveSceneAddress(cache, _sceneId, _sToken) == REDIS_DB_ERROR) {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("Scene::stop -- remove scene:%s location failed\n", _sceneId.c_str());
            } else {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
            }
        }
    }
}

bool Scene::enter(std::shared_ptr<GameObject> role) {
    if (!_running) {
        return false;
    }

    _roles.insert(std::make_pair(role->getRoleId(), obj));

    onEnter(role->getRoleId());
    return true;
}

bool Scene::leave(RoleId roleId) {
    auto it = _roles.find(roleId);
    if (it == _roles.end()) {
        return false;
    }

    onLeave(roleId);

    // 注意：如果onLeave中会进行协程切换，这里可能会已经被删除了（比如：场景进入了销毁流程），因此这里不用erase(it)而是用erase(roleId)
    _roles.erase(roleId);
    return true;
}

void Scene::regLocalEventHandle(const std::string &name, EventHandle handle) {
    _emiter.addEventHandle(name, handle);
}

void Scene::regGlobalEventHandle(const std::string &name, EventHandle handle) {
    uint32_t ref = _manager->regGlobalEventHandle(name, handle);
    _globalEventHandleRefs.push_back(ref);
}

void Scene::fireLocalEvent(const Event &event) {
    _emiter.fireEvent(event);
}

void Scene::fireGlobalEvent(const Event &event) {
    _manager->fireGlobalEvent(event);
}

void *Scene::heartbeatRoutine(void *arg) {
    SceneRoutineArg* routineArg = (SceneRoutineArg*)arg;
    std::shared_ptr<Scene> obj = std::move(routineArg->obj);
    delete routineArg;

    int failTimes = 0;
    while (obj->_running) {
        // 目前只有心跳和停服会销毁游戏对象
        obj->_cond.wait(TOKEN_HEARTBEAT_PERIOD);

        if (!obj->_running) {
            // 场景已被销毁
            break;
        }

        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] connect to cache failed\n", obj->_sceneId.c_str(), obj->_defId);

            failTimes++;
        } else {
            switch (RedisUtils::SetSceneAddressTTL(cache, obj->_sceneId, obj->_sToken)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCoreCache()->put(cache, true);
                    ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] check token failed for db error\n", obj->_sceneId.c_str(), obj->_defId);
                    failTimes++;
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] check session failed\n", obj->_sceneId.c_str(), obj->_defId);
                    failTimes += 3; // 直接失败
                }
                case REDIS_SUCCESS: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    failTimes = 0;
                }
            }
        }

        // 若超过3次设置失败，销毁场景对象
        if (failTimes >= 3) {
            if (obj->_running) {
                ERROR_LOG("Scene::heartbeatRoutine -- sceneId[%s] defId[%d] heartbeat failed and destroyed\n", obj->_sceneId.c_str(), obj->_defId);
                obj->_manager->removeScene(obj->_sceneId);
                assert(obj->_running = false);
            }
        }
    }

    return nullptr;
}

void *Scene::updateRoutine(void *arg) {
    SceneRoutineArg* routineArg = (SceneRoutineArg*)arg;
    std::shared_ptr<Scene> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;

    while (obj->_running) {
        obj->_cond.wait(g_SceneConfig.getUpdatePeriod());

        if (!obj->_running) {
            // 场景已被销毁
            break;
        }

        gettimeofday(&t, NULL);
        obj->update(t);
    }

    return nullptr;
}
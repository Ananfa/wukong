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

#include "redis_pool.h"
#include "redis_utils.h"
#include "const.h"

using namespace wukong;

RedisPool* RedisPool::create(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum) {
    RedisPool* pool = new RedisPool();
    pool->init(host, pwd, port, dbIndex, maxConnectNum);

    return pool;
}

void RedisPool::init(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum) {
    _redis = corpc::RedisConnectPool::create(host, pwd, port, dbIndex, maxConnectNum);

}

void RedisPool::addScripts(const std::map<std::string, std::string> &scripts) {
    InitScriptsContext* context = new InitScriptsContext;
    context->pool = this;
    context->scripts = scripts;

    // 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initScriptsRoutine, context);
}

void *RedisPool::initScriptsRoutine(void *arg) {
    InitScriptsContext* context = (InitScriptsContext *)arg;

    RedisPool *self = context->pool;
    std::map<std::string, std::string> scripts = std::move(context->scripts);

    delete context;

    redisContext *redis = self->_redis->proxy.take();
    if (!redis) {
        ERROR_LOG("RedisPool::initScriptRoutine -- connect to redis failed\n");
        return nullptr;
    }

    for (auto &pair : scripts) {
        if (self->_sha1Map.find(pair.first) != self->_sha1Map.end()) {
            WARN_LOG("RedisPool::initScriptRoutine -- script[%s] multiple load\n", pair.first.c_str());
            continue;
        }

        std::string sha1;
        switch (RedisUtils::LoadSha1(redis, pair.second, sha1)) {
            case REDIS_DB_ERROR: {
                self->_redis->proxy.put(redis, true);
                ERROR_LOG("RedisPool::initScriptRoutine -- script load failed for redis error\n");
                return nullptr;
            }
            case REDIS_FAIL: {
                WARN_LOG("RedisPool::initScriptRoutine -- script[%s] load from redis failed\n", pair.first.c_str());
                break;
            }
            case REDIS_SUCCESS: {
                DEBUG_LOG("RedisPool::initScriptRoutine -- load script[%s]\n", pair.first.c_str());
                self->_sha1Map.insert(std::make_pair(pair.first, sha1));
                break;
            }
        }
    }

    self->_redis->proxy.put(redis, false);

    return nullptr;
}

const char *RedisPool::getSha1(const char *sha1Name) {
    auto it = _sha1Map.find(sha1Name);
    if (it == _sha1Map.end()) {
        return nullptr;
    }

    return it->second.c_str();
}

redisContext *RedisPool::take() {
    return _redis->proxy.take();
}

void RedisPool::put(redisContext* redis, bool error) {
    _redis->proxy.put(redis, error);
}

bool RedisPoolManager::addPool(const std::string &poolName, RedisPool* pool) {
    if (_poolMap.find(poolName) != _poolMap.end()) {
        ERROR_LOG("RedisPoolManager::addPool -- multiple set pool[%s]\n", poolName.c_str());
        return false;
    }

    _poolMap.insert(std::make_pair(poolName, pool));
    return true;
}

RedisPool *RedisPoolManager::getPool(const std::string &poolName) {
    auto it = _poolMap.find(poolName);
    if (it == _poolMap.end()) {
        return nullptr;
    }

    return it->second;
}

bool RedisPoolManager::setCoreCache(const std::string &poolName) {
    if (_coreCache != nullptr) {
        ERROR_LOG("RedisPoolManager::setCoreCache -- multiple set core-cache pool[%s]\n", poolName.c_str());
        return false;
    }

    auto it = _poolMap.find(poolName);
    if (it == _poolMap.end()) {
        ERROR_LOG("RedisPoolManager::setCoreCache -- pool[%s] not exist\n", poolName.c_str());
        return false;
    }

    _coreCache = it->second;

    std::map<std::string, std::string> scripts;
    scripts[SET_PASSPORT_NAME] = SET_PASSPORT_CMD;
    scripts[CHECK_PASSPORT_NAME] = CHECK_PASSPORT_CMD;
    scripts[LOAD_ROLE_NAME] = LOAD_ROLE_CMD;
    scripts[SAVE_ROLE_NAME] = SAVE_ROLE_CMD;
    scripts[UPDATE_ROLE_NAME] = UPDATE_ROLE_CMD;
    scripts[SET_SESSION_NAME] = SET_SESSION_CMD;
    scripts[SET_SESSION_EXPIRE_NAME] = SET_SESSION_EXPIRE_CMD;
    scripts[REMOVE_SESSION_NAME] = REMOVE_SESSION_CMD;
    scripts[SET_RECORD_NAME] = SET_RECORD_CMD;
    scripts[REMOVE_RECORD_NAME] = REMOVE_RECORD_CMD;
    scripts[SET_RECORD_EXPIRE_NAME] = SET_RECORD_EXPIRE_CMD;
    scripts[SAVE_PROFILE_NAME] = SAVE_PROFILE_CMD;
    scripts[UPDATE_PROFILE_NAME] = UPDATE_PROFILE_CMD;
    scripts[SET_LOCATION_NAME] = SET_LOCATION_CMD;
    scripts[REMOVE_LOCATION_NAME] = REMOVE_LOCATION_CMD;
    scripts[UPDATE_LOCATION_NAME] = UPDATE_LOCATION_CMD;
    scripts[SET_LOCATION_EXPIRE_NAME] = SET_LOCATION_EXPIRE_CMD;
    scripts[SET_SCENE_LOCATION_NAME] = SET_SCENE_LOCATION_CMD;
    scripts[REMOVE_SCENE_LOCATION_NAME] = REMOVE_SCENE_LOCATION_CMD;
    scripts[SET_SCENE_LOCATION_EXPIRE_NAME] = SET_SCENE_LOCATION_EXPIRE_CMD;

    it->second->addScripts(scripts);
    return true;
}

bool RedisPoolManager::setCorePersist(const std::string &poolName) {
    if (_corePersist != nullptr) {
        ERROR_LOG("RedisPoolManager::setCorePersist -- multiple set core-persist pool[%s]\n", poolName.c_str());
        return false;
    }

    auto it = _poolMap.find(poolName);
    if (it == _poolMap.end()) {
        ERROR_LOG("RedisPoolManager::setCorePersist -- pool[%s] not exist\n", poolName.c_str());
        return false;
    }

    _corePersist = it->second;

    std::map<std::string, std::string> scripts;
    scripts[BIND_ROLE_NAME] = BIND_ROLE_CMD;

    it->second->addScripts(scripts);
    return true;
}

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

#include "cache_pool.h"
#include "const.h"

using namespace wukong;

void CachePool::init(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum) {
    _cache = corpc::RedisConnectPool::create(host, pwd, port, dbIndex, maxConnectNum);

    // 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initRoutine, this);
}

void *CachePool::initRoutine(void *arg) {
    CachePool *self = (CachePool *)arg;

    redisContext *cache = self->_cache->proxy.take();
    if (!cache) {
        ERROR_LOG("CachePool::initRoutine -- connect to cache failed\n");
        return nullptr;
    }

    // init _setPassportSha1
    redisReply *reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_PASSPORT_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-passport script load failed for cache error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-passport script load from cache failed\n");
        return nullptr;
    }

    self->_setPassportSha1 = reply->str;
    freeReplyObject(reply);

    // init _checkPassportSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", CHECK_PASSPORT_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- check-passport script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- check-passport script load failed\n");
        return nullptr;
    }

    self->_checkPassportSha1 = reply->str;
    freeReplyObject(reply);

    // init _loadRoleSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", LOAD_ROLE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- load-role script load failed for cache error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- load-role script load from cache failed\n");
        return nullptr;
    }

    self->_loadRoleSha1 = reply->str;
    freeReplyObject(reply);

    // init _saveRoleSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SAVE_ROLE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- save-role script load failed for cache error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- save-role script load from cache failed\n");
        return nullptr;
    }

    self->_saveRoleSha1 = reply->str;
    freeReplyObject(reply);

    // init _updateRoleSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", UPDATE_ROLE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- update-role script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- update-role script load failed\n");
        return nullptr;
    }

    self->_updateRoleSha1 = reply->str;
    freeReplyObject(reply);

    // init _setSessionSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_SESSION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-session script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-session script load failed\n");
        return nullptr;
    }

    self->_setSessionSha1 = reply->str;
    freeReplyObject(reply);

    // init _setSessionExpireSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_SESSION_EXPIRE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-session-expire script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-session-expire script load failed\n");
        return nullptr;
    }

    self->_setSessionExpireSha1 = reply->str;
    freeReplyObject(reply);

    // init _removeSessionSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", REMOVE_SESSION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- remove-session script load failed for cache error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- remove-session script load from cache failed\n");
        return nullptr;
    }

    self->_removeSessionSha1 = reply->str;
    freeReplyObject(reply);

    // init _setRecordSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_RECORD_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-record script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-record script load failed\n");
        return nullptr;
    }

    self->_setRecordSha1 = reply->str;
    freeReplyObject(reply);

    // init _removeRecordSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", REMOVE_RECORD_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- remove-record script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- remove-record script load failed\n");
        return nullptr;
    }

    self->_removeRecordSha1 = reply->str;
    freeReplyObject(reply);

    // init _setRecordExpireSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_RECORD_EXPIRE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-record-expire script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-record-expire script load failed\n");
        return nullptr;
    }

    self->_setRecordExpireSha1 = reply->str;
    freeReplyObject(reply);

    // init _saveProfileSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SAVE_PROFILE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- save-profile script load failed for cache error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- save-profile script load from cache failed\n");
        return nullptr;
    }

    self->_saveProfileSha1 = reply->str;
    freeReplyObject(reply);

    // init _updateProfileSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", UPDATE_PROFILE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- update-profile script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- update-profile script load failed\n");
        return nullptr;
    }

    self->_updateProfileSha1 = reply->str;
    freeReplyObject(reply);

    // init _setLocationSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-location script load failed\n");
        return nullptr;
    }

    self->_setLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _removeLocationSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", REMOVE_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- remove-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- remove-location script load failed\n");
        return nullptr;
    }

    self->_removeLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _updateLocationSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", UPDATE_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- update-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- update-location script load failed\n");
        return nullptr;
    }

    self->_updateLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _setLocationExpireSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_LOCATION_EXPIRE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-location-expire script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-location-expire script load failed\n");
        return nullptr;
    }

    self->_setLocationExpireSha1 = reply->str;
    freeReplyObject(reply);

    // init _setSceneLocationSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_SCENE_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-scene-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-scene-location script load failed\n");
        return nullptr;
    }

    self->_setSceneLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _removeSceneLocationSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", REMOVE_SCENE_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- remove-scene-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- remove-scene-location script load failed\n");
        return nullptr;
    }

    self->_removeSceneLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _setSceneLocationExpireSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_SCENE_LOCATION_EXPIRE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("CachePool::initRoutine -- set-scene-location-expire script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("CachePool::initRoutine -- set-scene-location-expire script load failed\n");
        return nullptr;
    }

    self->_setSceneLocationExpireSha1 = reply->str;
    freeReplyObject(reply);

    self->_cache->proxy.put(cache, false);

    return nullptr;
}

redisContext *CachePool::take() {
    return _cache->proxy.take();
}

void CachePool::put(redisContext* cache, bool error) {
    _cache->proxy.put(cache, error);
}

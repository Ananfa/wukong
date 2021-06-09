/*
 * Created by Xianke Liu on 2021/5/6.
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

#include "record_center.h"
#include "record_config.h"
#include "share/const.h"

using namespace wukong;

void *RecordCenter::initRoutine(void *arg) {
    RecordCenter *self = (RecordCenter *)arg;

    redisContext *cache = self->_cache->proxy.take();
    if (!cache) {
        ERROR_LOG("RecordCenter::initRoutine -- connect to cache failed\n");
        return nullptr;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_RECORD_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("RecordCenter::initRoutine -- set-record script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("RecordCenter::initRoutine -- set-record script load failed\n");
        return nullptr;
    }

    self->_setRecordSha1 = reply->str;
    freeReplyObject(reply);

    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_RECORD_EXPIRE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("RecordCenter::initRoutine -- set-record-expire script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("RecordCenter::initRoutine -- set-record-expire script load failed\n");
        return nullptr;
    }

    self->_setRecordExpireSha1 = reply->str;
    freeReplyObject(reply);

    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", UPDATE_PROFILE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("RecordCenter::initRoutine -- update-profile script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("RecordCenter::initRoutine -- update-profile script load failed\n");
        return nullptr;
    }

    self->_updateProfileSha1 = reply->str;
    freeReplyObject(reply);

    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", UPDATE_ROLE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("RecordCenter::initRoutine -- update-role script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("RecordCenter::initRoutine -- update-role script load failed\n");
        return nullptr;
    }

    self->_updateRoleSha1 = reply->str;
    freeReplyObject(reply);

    self->_cache->proxy.put(cache, false);
    
    return nullptr;
}

void RecordCenter::init() {
    _cache = corpc::RedisConnectPool::create(g_RecordConfig.getCache().host.c_str(), g_RecordConfig.getCache().port, g_RecordConfig.getCache().dbIndex, g_RecordConfig.getCache().maxConnect);
    _mysql = corpc::MysqlConnectPool::create(g_RecordConfig.getMysql().host.c_str(), g_RecordConfig.getMysql().user.c_str(), g_RecordConfig.getMysql().pwd.c_str(), g_RecordConfig.getMysql().dbName.c_str(), g_RecordConfig.getMysql().port, "", 0, g_RecordConfig.getMysql().maxConnect);

    // 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initRoutine, this);
}

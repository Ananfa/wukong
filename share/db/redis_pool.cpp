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
#include "const.h"

using namespace wukong;

void RedisPool::init(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum) {
    _redis = corpc::RedisConnectPool::create(host, pwd, port, dbIndex, maxConnectNum);

    // 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initRoutine, this);
}

void *RedisPool::initRoutine(void *arg) {
    RedisPool *self = (RedisPool *)arg;

    redisContext *redis = self->_redis->proxy.take();
    if (!redis) {
        ERROR_LOG("LoginHandlerMgr::initRoutine -- connect to redis failed\n");
        return nullptr;
    }

    // init _bindRoleSha1
    redisReply *reply = (redisReply *)redisCommand(redis, "SCRIPT LOAD %s", BIND_ROLE_CMD);
    if (!reply) {
        self->_redis->proxy.put(redis, true);
        ERROR_LOG("LoginHandlerMgr::initRoutine -- script load failed for redis error\n");
        return nullptr;
    }
    
    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_redis->proxy.put(redis, false);
        DEBUG_LOG("LoginHandlerMgr::initRoutine -- script load from redis failed\n");
        return nullptr;
    }

    self->_bindRoleSha1 = reply->str;
    freeReplyObject(reply);

    self->_redis->proxy.put(redis, false);

    return nullptr;
}

redisContext *RedisPool::take() {
    return _redis->proxy.take();
}

void RedisPool::put(redisContext* redis, bool error) {
    _redis->proxy.put(redis, error);
}

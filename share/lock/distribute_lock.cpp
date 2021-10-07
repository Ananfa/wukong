/*
 * Created by Xianke Liu on 2021/9/26.
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

#include "distribute_lock.h"
#include "corpc_rand.h"
#include "const.h"

using namespace corpc;
using namespace wukong;

DistributeLock::DistributeLock(RedisConnectPool *pool, std::string &lockKey): _pool(pool), _key(lockKey) {
    _identifier = randInt();
}

bool DistributeLock::lock() {
    if (_locked) {
        ERROR_LOG("DistributeLock::tryLock -- cant duplicate lock\n");
        return false;
    }

    redisContext *redis = _pool->proxy.take();
    if (!redis) {
        ERROR_LOG("DistributeLock::tryLock -- connect to redis failed\n");
        return false;
    }

    redisReply *reply = (redisReply *)redisCommand(redis, "SET Lock:%s %llu NX EX %d", _key.c_str(), _identifier, DISTRIBUTE_LOCK_EXPIRE);
    if (!reply) {
        _pool->proxy.put(redis, true);
        ERROR_LOG("DistributeLock::tryLock -- set lock failed for redis error\n");
        return false;
    } else if (reply->str == nullptr || strcmp(reply->str, "OK")) {
        freeReplyObject(reply);
        _pool->proxy.put(redis, false);
        return false;
    }

    freeReplyObject(reply);
    _pool->proxy.put(redis, false);
    _locked = true;
    return true;
}

bool DistributeLock::unlock() {
    if (!_locked) {
        return false;
    }

    redisContext *redis = _pool->proxy.take();
    if (!redis) {
        ERROR_LOG("DistributeLock::unlock -- connect to redis failed\n");
        return false;
    }

    redisReply *reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Lock:%s %llu", UNLOCK_CMD, _key.c_str(), _identifier);

    if (!reply) {
        _pool->proxy.put(redis, true);
        ERROR_LOG("DistributeLock::unlock -- remove lock from redis failed\n");
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        _pool->proxy.put(redis, true);
        ERROR_LOG("DistributeLock::unlock -- remove lock from redis failed for return type invalid\n");
        return false;
    }

    if (reply->integer == 0) {
        // 设置失败
        freeReplyObject(reply);
        _pool->proxy.put(redis, false);
        return false;
    }

    freeReplyObject(reply);
    _pool->proxy.put(redis, false);
    return true;
}

bool SimpleLockStrategy::tryLock(DistributeLock &lock) {
    if (lock.lock()) {
        return true;
    }

    for (int i = 1; i < _retryTimes; i++) {
        msleep(_interval);

        if (lock.lock()) {
            return true;
        }
    }

    return false;
}
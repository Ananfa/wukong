/*
 * Created by Xianke Liu on 2020/12/23.
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

#include "gateway_center.h"
#include "gateway_config.h"
#include "share/const.h"

using namespace wukong;

void GatewayCenter::init() {
	_cache = corpc::RedisConnectPool::create(g_GatewayConfig.getCache().host.c_str(), g_GatewayConfig.getCache().port, g_GatewayConfig.getCache().dbIndex, g_GatewayConfig.getCache().maxConnect);

	// 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initRoutine, this);
}

void *GatewayCenter::initRoutine(void *arg) {
    GatewayCenter *self = (GatewayCenter *)arg;

    redisContext *cache = self->_cache->proxy.take();
    if (!cache) {
        ERROR_LOG("GatewayCenter::initRoutine -- connect to cache failed\n");
        return nullptr;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", CHECK_SESSION_CMD);
    if (!reply) {
        ERROR_LOG("GatewayCenter::initRoutine -- check-session script load failed for db error\n");
        self->_cache->proxy.put(cache, true);
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        DEBUG_LOG("GatewayCenter::initRoutine -- check-session script load failed\n");
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        return nullptr;
    }

    self->_checkSessionSha1 = reply->str;
    freeReplyObject(reply);

    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_SESSION_EXPIRE_CMD);
    if (!reply) {
        ERROR_LOG("GatewayCenter::initRoutine -- set-session-expire script load failed for db error\n");
        self->_cache->proxy.put(cache, true);
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        DEBUG_LOG("GatewayCenter::initRoutine -- set-session-expire script load failed\n");
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        return nullptr;
    }

    self->_setSessionExpireSha1 = reply->str;
    freeReplyObject(reply);

    self->_cache->proxy.put(cache, false);
    
    return nullptr;
}

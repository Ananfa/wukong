/*
 * Created by Xianke Liu on 2021/1/15.
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

#include "game_center.h"
#include "share/const.h"

using namespace wukong;

void *GameCenter::initRoutine(void *arg) {
    GameCenter *self = (GameCenter *)arg;

    redisContext *cache = self->_cache->proxy.take();
    if (!cache) {
        ERROR_LOG("GameCenter::initRoutine -- connect to cache failed\n");
        return nullptr;
    }

    // init _setLocationSha1
    redisReply *reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("GameCenter::initRoutine -- set-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("GameCenter::initRoutine -- set-location script load failed\n");
        return nullptr;
    }

    self->_setLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _setLocationSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", REMOVE_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("GameCenter::initRoutine -- remove-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("GameCenter::initRoutine -- remove-location script load failed\n");
        return nullptr;
    }

    self->_removeLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _updateLocationSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", UPDATE_LOCATION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("GameCenter::initRoutine -- update-location script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("GameCenter::initRoutine -- update-location script load failed\n");
        return nullptr;
    }

    self->_updateLocationSha1 = reply->str;
    freeReplyObject(reply);

    // init _setLocationExpireSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_LOCATION_EXPIRE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("GameCenter::initRoutine -- set-location-expire script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("GameCenter::initRoutine -- set-location-expire script load failed\n");
        return nullptr;
    }

    self->_setLocationExpireSha1 = reply->str;
    freeReplyObject(reply);

    self->_cache->proxy.put(cache, false);
    
    return nullptr;
}

void GameCenter::init(GameServerType stype, uint32_t gameObjectUpdatePeriod, const char *dbHost, uint16_t dbPort, uint16_t dbIndex, uint32_t maxConnectNum) {
    _type = stype;
    _gameObjectUpdatePeriod = gameObjectUpdatePeriod;
    _cache = corpc::RedisConnectPool::create(dbHost, dbPort, dbIndex, maxConnectNum);

    // 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initRoutine, this);
}

bool GameCenter::registerMessage(int msgType,
                                    google::protobuf::Message *proto,
                                    bool needCoroutine,
                                    MessageHandle handle) {
    if (_registerMessageMap.find(msgType) != _registerMessageMap.end()) {
        return false;
    }

    RegisterMessageInfo info;
    info.proto = proto;
    info.needCoroutine = needCoroutine;
    info.handle = handle;
    
    _registerMessageMap.insert(std::make_pair(msgType, info));
    
    return true;
}

void GameCenter::handleMessage(std::shared_ptr<GameObject> obj, int msgType, uint16_t tag, const std::string &rawMsg) {
    auto iter = _registerMessageMap.find(msgType);
    if (iter == _registerMessageMap.end()) {
        ERROR_LOG("GameCenter::handleMessage -- unknown message type: %d\n", msgType);
        return;
    }

    google::protobuf::Message *msg = nullptr;
    if (iter->second.proto) {
        msg = iter->second.proto->New();
        if (!msg->ParseFromString(rawMsg)) {
            // 出错处理
            ERROR_LOG("GameCenter::handleMessage -- parse fail for message: %d\n", msgType);
            delete msg;
            return;
        } else {
            assert(!rawMsg.empty());
        }
    }

    std::shared_ptr<google::protobuf::Message> targetMsg = std::shared_ptr<google::protobuf::Message>(msg);

    if (iter->second.needCoroutine) {
        HandleMessageInfo *info = new HandleMessageInfo();
        info->obj = obj;
        info->msg = targetMsg;
        info->tag = tag;
        info->handle = iter->second.handle;

        RoutineEnvironment::startCoroutine(handleMessageRoutine, info);
        return;
    }

    iter->second.handle(obj, tag, targetMsg);
}

void *GameCenter::handleMessageRoutine(void *arg) {
    HandleMessageInfo *info = (HandleMessageInfo *)arg;

    info->handle(info->obj, info->tag, info->msg);
    delete info;
    
    return nullptr;
}

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

std::vector<LobbyClient::ServerInfo> GatewayCenter::_lobbyInfos;
std::mutex GatewayCenter::_lobbyInfosLock;
std::atomic<uint32_t> GatewayCenter::_lobbyInfosVersion(0);
thread_local std::vector<ServerWeightInfo> GatewayCenter::_t_lobbyInfos;
thread_local uint32_t GatewayCenter::_t_lobbyInfosVersion(0);
thread_local uint32_t GatewayCenter::_t_lobbyTotalWeight(0);

void *GatewayCenter::updateRoutine(void *arg) {
    GatewayCenter *self = (GatewayCenter *)arg;

    // 每秒检查是否有lobby的增加或减少，如果有马上刷新，否则每分钟刷新一次lobby的负载信息
    int i = 0;
    while (true) {
        i++;
        if (g_LobbyClient.stubChanged() || i >= 60) {
            self->updateLobbyInfos();
            i = 0;
        }

        sleep(1);
    }
    
    return nullptr;
}

void GatewayCenter::updateLobbyInfos() {
    std::vector<LobbyClient::ServerInfo> infos = g_LobbyClient.getServerInfos();
    {
        std::unique_lock<std::mutex> lock(_lobbyInfosLock);
        _lobbyInfos = std::move(infos);
        updateLobbyInfosVersion();
    }
    DEBUG_LOG("update lobby server info\n");
}

void *GatewayCenter::initRoutine(void *arg) {
    GatewayCenter *self = (GatewayCenter *)arg;

    redisContext *cache = self->_cache->proxy.take();
    if (!cache) {
        ERROR_LOG("GatewayCenter::initRoutine -- connect to cache failed\n");
        return nullptr;
    }

    // init _checkSessionSha1
    redisReply *reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", CHECK_SESSION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("GatewayCenter::initRoutine -- check-session script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("GatewayCenter::initRoutine -- check-session script load failed\n");
        return nullptr;
    }

    self->_checkSessionSha1 = reply->str;
    freeReplyObject(reply);

    // init _setSessionExpireSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", SET_SESSION_EXPIRE_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("GatewayCenter::initRoutine -- set-session-expire script load failed for db error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("GatewayCenter::initRoutine -- set-session-expire script load failed\n");
        return nullptr;
    }

    self->_setSessionExpireSha1 = reply->str;
    freeReplyObject(reply);

    // init _removeSessionSha1
    reply = (redisReply *)redisCommand(cache, "SCRIPT LOAD %s", REMOVE_SESSION_CMD);
    if (!reply) {
        self->_cache->proxy.put(cache, true);
        ERROR_LOG("GatewayCenter::initRoutine -- remove-session script load failed for cache error\n");
        return nullptr;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        self->_cache->proxy.put(cache, false);
        DEBUG_LOG("GatewayCenter::initRoutine -- remove-session script load from cache failed\n");
        return nullptr;
    }

    self->_removeSessionSha1 = reply->str;
    freeReplyObject(reply);


    self->_cache->proxy.put(cache, false);
    
    return nullptr;
}

void GatewayCenter::init() {
    _cache = corpc::RedisConnectPool::create(g_GatewayConfig.getCache().host.c_str(), g_GatewayConfig.getCache().port, g_GatewayConfig.getCache().dbIndex, g_GatewayConfig.getCache().maxConnect);

    RoutineEnvironment::startCoroutine(updateRoutine, this);

    // 初始化redis lua脚本sha1值
    RoutineEnvironment::startCoroutine(initRoutine, this);
}

bool GatewayCenter::randomLobbyServer(ServerId &serverId) {
    refreshLobbyInfos();
    size_t serverNum = _t_lobbyInfos.size();
    if (!serverNum) return false;
    
    uint32_t totalWeight = _t_lobbyTotalWeight;

    uint32_t i = 0;
    // 特殊处理, 前1000无视权重
    if (totalWeight <= 1000) {
        i = rand() % serverNum;
    } else {
        // rate from 1 to totalWeight
        uint32_t rate = rand() % totalWeight + 1;
        uint32_t until = 0;
        
        for (int j = 0; j < serverNum; j++) {
            until += _t_lobbyInfos[j].weight;
            if (rate <= until) {
                i = j;
                break;
            }
        }
    }

    serverId = _t_lobbyInfos[i].id;

    // 调整权重
    _t_lobbyTotalWeight += serverNum - 1;
    for (int j = 0; j < serverNum; j++) {
        _t_lobbyInfos[j].weight++;
    }
    _t_lobbyInfos[i].weight--;
    return true;
}

void GatewayCenter::refreshLobbyInfos() {
    if (_t_lobbyInfosVersion != _lobbyInfosVersion) {
        _t_lobbyInfos.clear();

        std::vector<LobbyClient::ServerInfo> lobbyInfos;

        {
            std::unique_lock<std::mutex> lock(_lobbyInfosLock);
            lobbyInfos = _lobbyInfos;
            _t_lobbyInfosVersion = _lobbyInfosVersion;
        }

        _t_lobbyInfos.reserve(lobbyInfos.size());
        uint32_t totalWeight = 0;
        for (auto &info : lobbyInfos) {
            totalWeight += info.weight;

            _t_lobbyInfos.push_back({info.id, info.weight});
        }
        
        _t_lobbyTotalWeight = totalWeight;
    }
}

/*
 * Created by Xianke Liu on 2021/1/7.
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
#include "gateway_object.h"
#include "gateway_object_manager.h"
#include "gateway_config.h"
#include "gateway_server.h"
#include "redis_pool.h"
#include "redis_utils.h"
//#include "lobby_client.h"
//#include "scene_client.h"
#include "agent_manager.h"
#include "lobby_agent.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

//bool GatewayObject::setGameServerStub(GameServerType gsType, ServerId sid) {
//    if (!gameServerStub_ || gameServerType_ != gsType || gameServerId_ != sid) {
//        gameServerType_ = gsType;
//        gameServerId_ = sid;
//
//        switch (gsType) {
//            case GAME_SERVER_TYPE_LOBBY: {
//                gameServerStub_ = g_LobbyClient.getGameServiceStub(sid);
//                break;
//            }
//            case GAME_SERVER_TYPE_SCENE: {
//                gameServerStub_ = g_SceneClient.getGameServiceStub(sid);
//                break;
//            }
//            default: {
//                ERROR_LOG("GatewayObject::setGameServerStub -- user[%llu] role[%llu] unknown game server[gsType:%d sid: %d]\n", userId_, roleId_, gsType, sid);
//                break;
//            }
//        }
//    }
//
//    if (!gameServerStub_) {
//        ERROR_LOG("GatewayObject::setGameServerStub -- user[%llu] role[%llu] game server[gsType:%d sid: %d] not found\n", userId_, roleId_, gsType, sid);
//        return false;
//    }
//
//    return true;
//}

void GatewayObject::setRelateServer(ServerType stype, ServerId sid) {
    relateServers_[stype] = sid;
}

ServerId GatewayObject::getRelateServerId(ServerType stype) {
    auto it = relateServers_.find(stype);
    if (it != relateServers_.end()) {
        return it->second;
    }

    return 0;
}

void GatewayObject::start() {
    running_ = true;

    GatewayObjectRoutineArg *arg = new GatewayObjectRoutineArg();
    arg->obj = shared_from_this();

    RoutineEnvironment::startCoroutine(heartbeatRoutine, arg);
}

void GatewayObject::stop() {
    if (running_) {
        DEBUG_LOG("GatewayObject::stop -- user[%llu] role[%llu] token:%s\n", userId_, roleId_, gToken_.c_str());
        running_ = false;

        cond_.broadcast();

        // 注意: 在这里直接进行redis操作会有协程切换，可能会导致一些流程同步问题（是否可以开新协程处理? 这样就不能保证stop返回session被清理）
        // 清除session
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("GatewayObject::stop -- user[%llu] role[%llu] connect to cache failed\n", userId_, roleId_);
            return;
        }

        if (RedisUtils::RemoveSession(cache, userId_, gToken_) == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GatewayObject::stop -- user[%llu] role[%llu] remove session failed", userId_, roleId_);
            return;
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
    }
}

void *GatewayObject::heartbeatRoutine( void *arg ) {
    GatewayObjectRoutineArg* routineArg = (GatewayObjectRoutineArg*)arg;
    std::shared_ptr<GatewayObject> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;
    gettimeofday(&t, NULL);

    obj->gameObjectHeartbeatExpire_ = t.tv_sec + TOKEN_TIMEOUT;

    while (obj->running_) {
        // 被踢出时触发，如果不是被踢出的情况则需要等到心跳周期
        obj->cond_.wait(TOKEN_HEARTBEAT_PERIOD);

        if (!obj->running_) {
            // 网关对象已被销毁
            return nullptr;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("GatewayObject::heartbeatRoutine -- user[%llu] role[%llu] connect to cache failed\n", obj->userId_, obj->roleId_);

            success = false;
        } else {
            switch (RedisUtils::SetSessionTTL(cache, obj->userId_, obj->gToken_)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCoreCache()->put(cache, true);
                    ERROR_LOG("GatewayObject::heartbeatRoutine -- user[%llu] role[%llu] check session failed for db error\n", obj->userId_, obj->roleId_);

                    success = false;
                    break;
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    ERROR_LOG("GatewayObject::heartbeatRoutine -- user[%llu] role[%llu] check session failed\n", obj->userId_, obj->roleId_);
                    break;
                }
                default: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    break;
                } 
            }
        }

        if (success) {
            // 判断游戏对象心跳是否过期
            gettimeofday(&t, NULL);
            success = t.tv_sec < obj->gameObjectHeartbeatExpire_;
            if (!success) {
                ERROR_LOG("GatewayObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat expired, expired time:%d\n", obj->userId_, obj->roleId_, obj->gameObjectHeartbeatExpire_);
            }
        }

        // 若设置超时不成功，销毁网关对象
        if (!success) {
            if (obj->running_) {
                if (!g_GatewayObjectManager.removeGatewayObject(obj->userId_)) {
                    assert(false);
                    ERROR_LOG("GatewayObject::heartbeatRoutine -- user[%llu] role[%llu] remove route object failed\n", obj->userId_, obj->roleId_);
                }

                obj->running_ = false;
            }
        }
    }

    return nullptr;
}

void GatewayObject::forwardIn(int32_t msgType, uint16_t tag, std::shared_ptr<std::string> &rawMsg) {
    ServerType stype = (msgType >> 16) & 0xFFFF;

    ServerId sid = getRelateServerId(stype);
    if (sid == 0) {
        ERROR_LOG("GatewayObject::forwardIn -- user[%llu] role[%llu] server[%d] not bound\n", userId_, roleId_, stype);
        return;
    }

    GameAgent *agent = (GameAgent*)g_AgentManager.getAgent(stype);

    if (!agent) {
        ERROR_LOG("GatewayObject::forwardIn -- game agent[%d] not found\n", stype);
        return;
    }

    agent->forwardIn(sid, msgType, tag, roleId_, rawMsg);
}

void GatewayObject::enterGame() {
    DEBUG_LOG("GatewayObject::enterGame -- user: %d\n", userId_);

    ServerId sid = getRelateServerId(SERVER_TYPE_LOBBY);
    if (sid == 0) {
        ERROR_LOG("GatewayObject::enterGame -- user[%llu] role[%llu] lobby not bound\n", userId_, roleId_);
        return;
    }

    LobbyAgent *agent = (LobbyAgent*)g_AgentManager.getAgent(SERVER_TYPE_LOBBY);

    if (!agent) {
        ERROR_LOG("GatewayObject::enterGame -- lobby agent not found\n");
        return;
    }

    agent->enterGame(sid, roleId_, lToken_, g_GatewayConfig.getId());
}

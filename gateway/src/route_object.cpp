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
#include "route_object.h"
#include "gateway_center.h"
#include "gateway_manager.h"
#include "lobby_client.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

static void callDoneHandle(::google::protobuf::Message *request, Controller *controller) {
    delete controller;
    delete request;
}

bool RouteObject::setGameServerStub(GameServerType stype, ServerId sid) {
    if (!_gameServerStub || _gameServerType != stype || _gameServerId != sid) {
        _gameServerType = stype;
        _gameServerId = sid;

        switch (stype) {
            case GAME_SERVER_TYPE_LOBBY:
                _gameServerStub = g_LobbyClient.getGameServiceStub(sid);

                break;
            case GAME_SERVER_TYPE_SCENE:
                // TODO:
            default:
                ERROR_LOG("RouteObject::setGameServerStub -- user %d unknown game server[stype:%d sid: %d]\n", _userId, stype, sid);
                break;
        }
    }

    if (!_gameServerStub) {
        ERROR_LOG("RouteObject::setGameServerStub -- user %d game server[stype:%d sid: %d] not found\n", _userId, stype, sid);
        return false;
    }

    return true;
}

void RouteObject::start() {
    _running = true;

    RouteObjectHeartbeatInfo *info = new RouteObjectHeartbeatInfo();
    info->ro = shared_from_this();

    RoutineEnvironment::startCoroutine(heartbeatRoutine, info);
}

void RouteObject::stop() {
    _running = false;
}

void *RouteObject::heartbeatRoutine( void *arg ) {
    RouteObjectHeartbeatInfo* info = (RouteObjectHeartbeatInfo*)arg;
    std::shared_ptr<RouteObject> ro = std::move(info->ro);
    delete info;

    struct timeval t;
    gettimeofday(&t, NULL);

    ro->_gameObjectHeartbeatExpire = t.tv_sec + 60;

    while (ro->_running) {
        sleep(20); // 路由对象销毁后，心跳协程最多停留20秒（这段时间会占用一点系统资源）

        if (!ro->_running) {
            // 路由对象已被销毁
            return nullptr;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_GatewayCenter.getCachePool()->proxy.take();
        if (!cache) {
            ERROR_LOG("RouteObject::heartbeatRoutine -- user %d connect to cache failed\n", ro->_userId);

            success = false;
        } else {
            redisReply *reply;
            if (g_GatewayCenter.setSessionExpireSha1().empty()) {
                reply = (redisReply *)redisCommand(cache, "eval %s 1 session:%d %s %d", SET_SESSION_EXPIRE_CMD, ro->_userId, ro->_token.c_str(), 60);
            } else {
                reply = (redisReply *)redisCommand(cache, "eval %s 1 session:%d %s %d", g_GatewayCenter.setSessionExpireSha1(), ro->_userId, ro->_token.c_str(), 60);
            }
            
            if (!reply) {
                g_GatewayCenter.getCachePool()->proxy.put(cache, true);
                ERROR_LOG("RouteObject::heartbeatRoutine -- user %d check session failed for db error\n", ro->_userId);

                success = false;
            } else {
                success = reply->integer == 1;
                freeReplyObject(reply);
                g_GatewayCenter.getCachePool()->proxy.put(cache, false);

                if (!success) {
                    ERROR_LOG("RouteObject::heartbeatRoutine -- user %d check session failed\n", ro->_userId);
                }
            }
        }

        if (success) {
            // TODO: 判断游戏对象心跳是否过期
            gettimeofday(&t, NULL);
            success = t.tv_sec < ro->_gameObjectHeartbeatExpire;
            if (!success) {
                ERROR_LOG("RouteObject::heartbeatRoutine -- user %d heartbeat expired\n", ro->_userId);
            }
        }

        // 若设置超时不成功，销毁路由对象
        if (!success) {
            if (ro->_running) {
                if (!ro->_manager->removeRouteObject(ro->_userId)) {
                    assert(false);
                    ERROR_LOG("RouteObject::heartbeatRoutine -- user %d remove route object failed\n", ro->_userId);
                }

                ro->_running = false;
            }
        }
    }

    return nullptr;
}

void RouteObject::forwardIn(int16_t type, uint16_t tag, std::shared_ptr<std::string> &rawMsg) {
    if (!_gameServerStub) {
        ERROR_LOG("RouteObject server stub not avaliable\n");
        return;
    }
    
    pb::ForwardRequest *request = new pb::ForwardRequest();
    Controller *controller = new Controller();
    request->set_type(type);

    if (tag != 0) {
        request->set_tag(tag);
    }

    request->add_ids(_roleId);
    
    if (rawMsg && !rawMsg->empty()) {
        request->set_rawmsg(rawMsg->c_str());
    }
    
    _gameServerStub->forward(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

void RouteObject::enterGame() {
    if (!_gameServerStub) {
        ERROR_LOG("RouteObject server stub not avaliable\n");
        return;
    }
    
    pb::Uint32Value *request = new pb::Uint32Value();
    Controller *controller = new Controller();
    request->set_value(_roleId);

    _gameServerStub->enterGame(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

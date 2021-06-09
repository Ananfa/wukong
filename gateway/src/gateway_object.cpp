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
#include "gateway_center.h"
#include "gateway_manager.h"
#include "gateway_server.h"
#include "lobby_client.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

static void callDoneHandle(::google::protobuf::Message *request, Controller *controller) {
    delete controller;
    delete request;
}

bool GatewayObject::setGameServerStub(GameServerType gsType, ServerId sid) {
    if (!_gameServerStub || _gameServerType != gsType || _gameServerId != sid) {
        _gameServerType = gsType;
        _gameServerId = sid;

        GameClient *gameClient = g_GatewayServer.getGameClient(gsType);
        if (gameClient) {
            _gameServerStub = gameClient->getGameServiceStub(sid);
        } else {
            ERROR_LOG("GatewayObject::setGameServerStub -- user %d unknown game server[gsType:%d sid: %d]\n", _userId, gsType, sid);
        }
    }

    if (!_gameServerStub) {
        ERROR_LOG("GatewayObject::setGameServerStub -- user %d game server[gsType:%d sid: %d] not found\n", _userId, gsType, sid);
        return false;
    }

    return true;
}

void GatewayObject::start() {
    _running = true;

    GatewayObjectRoutineArg *arg = new GatewayObjectRoutineArg();
    arg->obj = shared_from_this();

    RoutineEnvironment::startCoroutine(heartbeatRoutine, arg);
}

void GatewayObject::stop() {
    _running = false;
}

void *GatewayObject::heartbeatRoutine( void *arg ) {
    GatewayObjectRoutineArg* routineArg = (GatewayObjectRoutineArg*)arg;
    std::shared_ptr<GatewayObject> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;
    gettimeofday(&t, NULL);

    obj->_gameObjectHeartbeatExpire = t.tv_sec + TOKEN_TIMEOUT;

    while (obj->_running) {
        sleep(TOKEN_HEARTBEAT_PERIOD); // 网关对象销毁后，心跳协程最多停留20秒（这段时间会占用一点系统资源）

        if (!obj->_running) {
            // 网关对象已被销毁
            return nullptr;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_GatewayCenter.getCachePool()->proxy.take();
        if (!cache) {
            ERROR_LOG("GatewayObject::heartbeatRoutine -- user %d connect to cache failed\n", obj->_userId);

            success = false;
        } else {
            redisReply *reply;
            if (g_GatewayCenter.setSessionExpireSha1().empty()) {
                reply = (redisReply *)redisCommand(cache, "EVAL %s 1 Session:%d %s %d", SET_SESSION_EXPIRE_CMD, obj->_userId, obj->_gToken.c_str(), TOKEN_TIMEOUT);
            } else {
                reply = (redisReply *)redisCommand(cache, "EVALSHA %s 1 Session:%d %s %d", g_GatewayCenter.setSessionExpireSha1(), obj->_userId, obj->_gToken.c_str(), TOKEN_TIMEOUT);
            }
            
            if (!reply) {
                g_GatewayCenter.getCachePool()->proxy.put(cache, true);
                ERROR_LOG("GatewayObject::heartbeatRoutine -- user %d check session failed for db error\n", obj->_userId);

                success = false;
            } else {
                success = reply->integer == 1;
                freeReplyObject(reply);
                g_GatewayCenter.getCachePool()->proxy.put(cache, false);

                if (!success) {
                    ERROR_LOG("GatewayObject::heartbeatRoutine -- user %d check session failed\n", obj->_userId);
                }
            }
        }

        if (success) {
            // 判断游戏对象心跳是否过期
            gettimeofday(&t, NULL);
            success = t.tv_sec < obj->_gameObjectHeartbeatExpire;
            if (!success) {
                ERROR_LOG("GatewayObject::heartbeatRoutine -- user %d heartbeat expired\n", obj->_userId);
            }
        }

        // 若设置超时不成功，销毁网关对象
        if (!success) {
            if (obj->_running) {
                if (!obj->_manager->removeGatewayObject(obj->_userId)) {
                    assert(false);
                    ERROR_LOG("GatewayObject::heartbeatRoutine -- user %d remove route object failed\n", obj->_userId);
                }

                obj->_running = false;
            }
        }
    }

    return nullptr;
}

void GatewayObject::forwardIn(int16_t type, uint16_t tag, std::shared_ptr<std::string> &rawMsg) {
    if (!_gameServerStub) {
        ERROR_LOG("GatewayObject server stub not avaliable\n");
        return;
    }
    
    pb::ForwardInRequest *request = new pb::ForwardInRequest();
    Controller *controller = new Controller();
    request->set_type(type);

    if (tag != 0) {
        request->set_tag(tag);
    }

    request->add_ids(_roleId);
    
    if (rawMsg && !rawMsg->empty()) {
        request->set_rawmsg(rawMsg->c_str());
    }
    
    _gameServerStub->forwardIn(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

void GatewayObject::enterGame() {
    if (!_gameServerStub) {
        ERROR_LOG("GatewayObject server stub not avaliable\n");
        return;
    }
    
    pb::EnterGameRequest *request = new pb::EnterGameRequest();
    Controller *controller = new Controller();
    request->set_roleid(_roleId);
    request->set_ltoken(_lToken);
    request->set_gatewayid(_manager->getId());

    _gameServerStub->enterGame(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

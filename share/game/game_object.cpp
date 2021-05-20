/*
 * Created by Xianke Liu on 2021/1/19.
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
#include "game_object.h"
#include "game_center.h"
#include "game_object_manager.h"
#include "gateway_client.h"
#include "record_client.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

static void callDoneHandle(::google::protobuf::Message *request, Controller *controller) {
    delete controller;
    delete request;
}

bool GameObject::setGatewayServerStub(ServerId sid) {
    if (!_gatewayServerStub || _gatewayId != sid) {
        _gatewayId = sid;

        _gatewayServerStub = g_GatewayClient.getStub(sid);
    }

    if (!_gatewayServerStub) {
        ERROR_LOG("GameObject::setGatewayServerStub -- user %d gateway server[sid: %d] not found\n", _userId, sid);
        return false;
    }

    return true;
}

bool GameObject::setRecordServerStub(ServerId sid) {
    if (!_recordServerStub || _recordId != sid) {
        _recordId = sid;

        _recordServerStub = g_RecordClient.getStub(sid);
    }

    if (!_recordServerStub) {
        ERROR_LOG("GameObject::setRecordServerStub -- user %d record server[sid: %d] not found\n", _userId, sid);
        return false;
    }

    return true;
}

void GameObject::start() {
    _running = true;

    {
        GameObjectRoutineArg *arg = new GameObjectRoutineArg();
        arg->obj = shared_from_this();
        RoutineEnvironment::startCoroutine(heartbeatRoutine, arg);
    }

    // 启动定期存盘协程（每隔一段时间将脏数据同步给记录服，协程中退出循环时也会执行一次）
    {
        GameObjectRoutineArg *arg = new GameObjectRoutineArg();
        arg->obj = shared_from_this();
        RoutineEnvironment::startCoroutine(syncRoutine, arg);
    }

    // 根据配置启动周期处理协程（执行周期更新逻辑）
    if (g_GameCenter.getGameObjectUpdatePeriod() > 0) {
        GameObjectRoutineArg *arg = new GameObjectRoutineArg();
        arg->obj = shared_from_this();
        RoutineEnvironment::startCoroutine(updateRoutine, arg);
    }
}

void GameObject::stop() {
    _running = false;
}

void GameObject::forwardOut(int32_t type, uint16_t tag, const std::vector<std::pair<UserId, uint32_t>> &targets, const std::string &msg) {
    if (!_gatewayServerStub) {
        ERROR_LOG("GameObject::forwardOut -- gateway stub not set\n");
        return;
    }
    
    pb::ForwardOutRequest *request = new pb::ForwardOutRequest();
    Controller *controller = new Controller();
    request->set_type(type);
    request->set_tag(tag);
    for (auto it = targets.begin(); it != targets.end(); ++it) {
        ::wukong::pb::ForwardOutTarget* target = request->add_targets();
        target->set_userid(it->first);
        target->set_ltoken(it->second);
    }
    
    if (!msg.empty()) {
        request->set_rawmsg(msg);
    }
    
    _gatewayServerStub->forwardOut(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

bool GameObject::reportGameObjectPos() {
    if (!_gatewayServerStub) {
        ERROR_LOG("GameObject::setGameObjectPos -- gateway stub not set\n");
        return false;
    }

    pb::SetGameObjectPosRequest *request = new pb::SetGameObjectPosRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_userid(_userId);
    request->set_ltoken(_lToken);
    request->set_servertype(g_GameCenter.getType());
    request->set_serverid(_manager->getId());
    _gatewayServerStub->setGameObjectPos(controller, request, response, nullptr);
    
    bool ret;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        ret = false;
    } else {
        ret = response->value();
    }
    
    delete controller;
    delete response;
    delete request;

    return ret;
}

bool GameObject::heartbeatToGateway() {
    if (!_gatewayServerStub) {
        ERROR_LOG("GameObject::heartbeatToGateway -- gateway stub not set\n");
        return false;
    }

    pb::GSHeartbeatRequest *request = new pb::GSHeartbeatRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_userid(_userId);
    request->set_ltoken(_lToken);
    _gatewayServerStub->heartbeat(controller, request, response, nullptr);
    
    bool ret;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        ret = false;
    } else {
        ret = response->value();
    }
    
    delete controller;
    delete response;
    delete request;

    return ret;
}

bool GameObject::sync(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) {
    if (!_recordServerStub) {
        ERROR_LOG("GameObject::sync -- record stub not set\n");
        return false;
    }

    pb::SyncRequest *request = new pb::SyncRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_ltoken(_lToken);
    request->set_roleid(_roleId);
    for (auto it = datas.begin(); it != datas.end(); ++it) {
        auto data = request->add_datas();
        data->set_key(it->first);
        data->set_value(it->second);
    }
    for (auto it = removes.begin(); it != removes.end(); ++it) {
        request->add_removes(*it);
    }

    _recordServerStub->sync(controller, request, response, nullptr);

    bool result = false;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else {
        result = response->value();
    }

    delete controller;
    delete response;
    delete request;

    return result;
}

bool GameObject::heartbeatToRecord() {
    if (!_recordServerStub) {
        ERROR_LOG("GameObject::heartbeatToRecord -- record stub not set\n");
        return false;
    }

    pb::RSHeartbeatRequest *request = new pb::RSHeartbeatRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_roleid(_roleId);
    request->set_ltoken(_lToken);
    _recordServerStub->heartbeat(controller, request, response, nullptr);
    
    bool ret;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        ret = false;
    } else {
        ret = response->value();
    }
    
    delete controller;
    delete response;
    delete request;

    return ret;
}

void *GameObject::heartbeatRoutine( void *arg ) {
    GameObjectRoutineArg* routineArg = (GameObjectRoutineArg*)arg;
    std::shared_ptr<GameObject> obj = std::move(routineArg->obj);
    delete routineArg;

    while (obj->_running) {
        sleep(TOKEN_HEARTBEAT_PERIOD); // 游戏对象销毁后，心跳协程最多停留20秒（这段时间会占用一点系统资源）

        if (!obj->_running) {
            // 游戏对象已被销毁
            break;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_GameCenter.getCachePool()->proxy.take();
        if (!cache) {
            ERROR_LOG("GameObject::heartbeatRoutine -- user %d role %d connect to cache failed\n", obj->_userId, obj->_roleId);

            success = false;
        } else {
            redisReply *reply;
            if (g_GameCenter.setLocationExpireSha1().empty()) {
                reply = (redisReply *)redisCommand(cache, "EVAL %s 1 location:%d %d %d", SET_LOCATION_EXPIRE_CMD, obj->_roleId, obj->_lToken, TOKEN_TIMEOUT);
            } else {
                reply = (redisReply *)redisCommand(cache, "EVALSHA %s 1 location:%d %d %d", g_GameCenter.setLocationExpireSha1(), obj->_roleId, obj->_lToken, TOKEN_TIMEOUT);
            }
            
            if (!reply) {
                g_GameCenter.getCachePool()->proxy.put(cache, true);
                ERROR_LOG("GameObject::heartbeatRoutine -- user %d role %d check session failed for db error\n", obj->_userId, obj->_roleId);

                success = false;
            } else {
                success = reply->integer == 1;
                freeReplyObject(reply);
                g_GameCenter.getCachePool()->proxy.put(cache, false);

                if (!success) {
                    ERROR_LOG("GameObject::heartbeatRoutine -- user %d role %d check session failed\n", obj->_userId, obj->_roleId);
                }
            }
        }

        if (success) {
            // 对网关对象进行心跳
            success = obj->heartbeatToGateway();
        }

        if (success) {
            // 对记录对象进行心跳
            success = obj->heartbeatToRecord();
        }

        // 若设置超时不成功，销毁游戏对象
        if (!success) {
            if (obj->_running) {
                if (!obj->_manager->remove(obj->_roleId)) {
                    assert(false);
                    ERROR_LOG("GameObject::heartbeatRoutine -- user %d role %d remove game object failed\n", obj->_userId, obj->_roleId);
                }

                obj->_running = false;
            }
        }
    }

    return nullptr;
}

void *GameObject::syncRoutine(void *arg) {
    GameObjectRoutineArg* routineArg = (GameObjectRoutineArg*)arg;
    std::shared_ptr<GameObject> obj = std::move(routineArg->obj);
    delete routineArg;

    std::list<std::pair<std::string, std::string>> syncDatas;
    std::list<std::string> removes;

    while (obj->_running) {
        for (int i = 0; i < SYNC_PERIOD; i++) {
            sleep(1);

            if (!obj->_running) {
                break;
            }
        }

        // 向记录服同步数据（销毁前也应该将脏数据同步给记录服）
        obj->buildSyncDatas(syncDatas, removes);
        if (!syncDatas.empty() || !removes.empty()) {
            obj->sync(syncDatas, removes);
            syncDatas.clear();
            removes.clear();
        }

    }
}

void *GameObject::updateRoutine(void *arg) {
    GameObjectRoutineArg* routineArg = (GameObjectRoutineArg*)arg;
    std::shared_ptr<GameObject> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;

    while (obj->_running) {
        msleep(g_GameCenter.getGameObjectUpdatePeriod());

        if (!obj->_running) {
            // 游戏对象已被销毁
            break;
        }

        gettimeofday(&t, NULL);
        obj->update(t.tv_sec);
    }
}
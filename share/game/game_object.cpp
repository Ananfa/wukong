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
#include "redis_pool.h"
#include "redis_utils.h"
#include "game_object_manager.h"
#include "gateway_client.h"
#include "record_client.h"
#include "share/const.h"

using namespace wukong;

GameObject::~GameObject() {}

bool GameObject::setGatewayServerStub(ServerId sid) {
    if (!gatewayServerStub_ || gatewayId_ != sid) {
        gatewayId_ = sid;

        gatewayServerStub_ = g_GatewayClient.getStub(sid);
    }

    if (!gatewayServerStub_) {
        ERROR_LOG("GameObject::setGatewayServerStub -- user[%llu] role[%llu] gateway server[sid: %d] not found\n", userId_, roleId_, sid);
        return false;
    }

    return true;
}

bool GameObject::setRecordServerStub(ServerId sid) {
    if (!recordServerStub_ || recordId_ != sid) {
        recordId_ = sid;

        recordServerStub_ = g_RecordClient.getStub(sid);
    }

    if (!recordServerStub_) {
        ERROR_LOG("GameObject::setRecordServerStub -- user[%llu] role[%llu] record server[sid: %d] not found\n", userId_, roleId_, sid);
        return false;
    }

    return true;
}

void GameObject::start() {
    running_ = true;

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

    onStart();
}

void GameObject::stop() {
    if (running_) {
        DEBUG_LOG("GameObject::stop user[%llu] role[%llu] token:%s\n", userId_, roleId_, lToken_.c_str());
        running_ = false;

        // 若不清emiter，会导致shared_ptr循环引用问题
        emiter_.clear();

        for (auto ref : globalEventHandleRefs_) {
            manager_->unregGlobalEventHandle(ref);
        }
        globalEventHandleRefs_.clear();

        onDestory();

        cond_.broadcast();

        // TODO: 在这里直接进行redis操作会有协程切换，导致一些流程同步问题，需要考虑一下是否需要换地方调用
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("GameObject::stop -- user[%llu] role[%llu] connect to cache failed\n", userId_, roleId_);
            return;
        }

        if (RedisUtils::RemoveGameObjectAddress(cache, roleId_, lToken_) == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GameObject::stop -- user[%llu] role[%llu] remove location failed", userId_, roleId_);
            return;
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
    }
}

void GameObject::enterGame() {
    gwHeartbeatFailCount_ = 0; // 重置心跳
    enterTimes_++;
    onEnterGame();
}

void GameObject::leaveGame() {
    manager_->leaveGame(roleId_);
}

void GameObject::regLocalEventHandle(const std::string &name, EventHandle handle) {
    emiter_.addEventHandle(name, handle);
}

void GameObject::regGlobalEventHandle(const std::string &name, EventHandle handle) {
    uint32_t ref = manager_->regGlobalEventHandle(name, handle);
    globalEventHandleRefs_.push_back(ref);
}

void GameObject::fireLocalEvent(const Event &event) {
    emiter_.fireEvent(event);
}

void GameObject::fireGlobalEvent(const Event &event) {
    manager_->fireGlobalEvent(event);
}

void GameObject::send(int32_t type, uint16_t tag, const std::string &msg) {
    if (!gatewayServerStub_) {
        ERROR_LOG("GameObject::send(raw) -- user[%llu] role[%llu] type %d gateway stub not set\n", userId_, roleId_, type);
        return;
    }
    
    pb::ForwardOutRequest *request = new pb::ForwardOutRequest();
    request->set_serverid(gatewayId_);
    request->set_type(type);
    request->set_tag(tag);

    ::wukong::pb::ForwardOutTarget* target = request->add_targets();
    target->set_userid(userId_);
    target->set_ltoken(lToken_);
    
    if (!msg.empty()) {
        request->set_rawmsg(msg);
    }

    DEBUG_LOG("GameObject::send -- type:%d userId:%d\n", type, userId_);
    
    gatewayServerStub_->forwardOut(nullptr, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request));
}

void GameObject::send(int32_t type, uint16_t tag, google::protobuf::Message &msg) {
    if (!gatewayServerStub_) {
        ERROR_LOG("GameObject::send -- user[%llu] role[%llu] type %d gateway stub not set\n", userId_, roleId_, type);
        return;
    }

    std::string bufStr(msg.ByteSizeLong(), 0);
    uint8_t *buf = (uint8_t *)bufStr.data();
    msg.SerializeWithCachedSizesToArray(buf);

    send(type, tag, bufStr);
}

int GameObject::reportGameObjectPos() {
    if (!gatewayServerStub_) {
        ERROR_LOG("GameObject::setGameObjectPos -- user[%llu] role[%llu] gateway stub not set\n", userId_, roleId_);
        return -1;
    }

    pb::SetGameObjectPosRequest *request = new pb::SetGameObjectPosRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_serverid(gatewayId_);
    request->set_userid(userId_);
    request->set_roleid(roleId_);
    request->set_ltoken(lToken_);
    request->set_gstype(g_GameCenter.getType());
    request->set_gsid(manager_->getId());
    gatewayServerStub_->setGameObjectPos(controller, request, response, nullptr);
    
    int ret;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        ret = -2;
    } else {
        ret = response->value()?1:0;
    }
    
    delete controller;
    delete response;
    delete request;

    return ret;
}

int GameObject::heartbeatToGateway() {
    if (!gatewayServerStub_) {
        ERROR_LOG("GameObject::heartbeatToGateway -- user[%llu] role[%llu] gateway stub not set\n", userId_, roleId_);
        return -1;
    }

    pb::GSHeartbeatRequest *request = new pb::GSHeartbeatRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_serverid(gatewayId_);
    request->set_userid(userId_);
    request->set_ltoken(lToken_);
    gatewayServerStub_->heartbeat(controller, request, response, nullptr);
    
    int ret;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        ret = -2;
    } else {
        ret = response->value()?1:0;
    }
    
    delete controller;
    delete response;
    delete request;

    return ret;
}

int GameObject::heartbeatToRecord() {
    if (!recordServerStub_) {
        ERROR_LOG("GameObject::heartbeatToRecord -- user[%llu] role[%llu] record stub not set\n", userId_, roleId_);
        return -1;
    }

    pb::RSHeartbeatRequest *request = new pb::RSHeartbeatRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_serverid(recordId_);
    request->set_roleid(roleId_);
    request->set_ltoken(lToken_);
    recordServerStub_->heartbeat(controller, request, response, nullptr);
    
    int ret;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        ret = -2;
    } else {
        ret = response->value()?1:0;
    }
    
    delete controller;
    delete response;
    delete request;

    return ret;
}

bool GameObject::sync(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) {
    if (!recordServerStub_) {
        ERROR_LOG("GameObject::sync -- user[%llu] role[%llu] record stub not set\n", userId_, roleId_);
        return false;
    }

    pb::SyncRequest *request = new pb::SyncRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_serverid(recordId_);
    request->set_ltoken(lToken_);
    request->set_roleid(roleId_);
    for (auto &d : datas) {
        auto data = request->add_datas();
        data->set_key(d.first);
        data->set_value(d.second);
    }
    for (auto &r : removes) {
        request->add_removes(r);
    }

    recordServerStub_->sync(controller, request, response, nullptr);

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

void *GameObject::heartbeatRoutine( void *arg ) {
    GameObjectRoutineArg* routineArg = (GameObjectRoutineArg*)arg;
    std::shared_ptr<GameObject> obj = std::move(routineArg->obj);
    delete routineArg;

    while (obj->running_) {
        // 目前只有心跳和停服会销毁游戏对象
        obj->cond_.wait(TOKEN_HEARTBEAT_PERIOD);

        if (!obj->running_) {
            // 游戏对象已被销毁
            break;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] connect to cache failed\n", obj->userId_, obj->roleId_);

            success = false;
        } else {
            switch (RedisUtils::SetGameObjectAddressTTL(cache, obj->roleId_, obj->lToken_)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCoreCache()->put(cache, true);
                    ERROR_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] check session failed for db error\n", obj->userId_, obj->roleId_);
                    success = false;
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    ERROR_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] check session failed\n", obj->userId_, obj->roleId_);
                    success = false;
                }
                case REDIS_SUCCESS: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    success = true;
                }
            }
        }

        // TODO: 对gateway对象进行心跳改为失败时发通知不直接导致游戏对象删除，交由上层决定是否删除游戏对象
        if (success) {
            if (obj->gatewayServerStub_) {
                // 对网关对象进行心跳
                // 三次心跳不到gateway对象才销毁
                int curEnterTimes = obj->enterTimes_;
                switch (obj->heartbeatToGateway()) {
                    case -2: {// rpc出错（心跳超时）
                        obj->gwHeartbeatFailCount_++;
                        WARN_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat timeout, count:%d\n", obj->userId_, obj->roleId_, obj->gwHeartbeatFailCount_);
                        break;
                    }
                    case 0: {// gateway对象不存在
                        // 注意：
                        // 这里有个问题：重登时正好gameobj向gatewayobj心跳，在gatewayobj的旧对象销毁新对象重建过程中刚好心跳请求到达，因此返回心跳失败--找不到gatewayobj，
                        // 在心跳RPC结果返回过程中，新gatewayobj创建完成并且通知gameobj连接建立，gameobj通知客户端进入游戏完成，（此处进行心跳失败处理将gateway对象stub清除了），
                        // 导致gameobj错误的丢掉了gatewayobj的连接。
                        // 解决方法：gameobj中增加一个登录计数值，心跳前和心跳后的登录计数值一样才是真的断线
                        if (curEnterTimes == obj->enterTimes_) {
                            WARN_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to gw failed for gw object not exit\n", obj->userId_, obj->roleId_);

                            obj->gatewayServerStub_ = nullptr;
                            obj->onOffline(); // 通知上层玩家离线
                        } else {
                            WARN_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to gw failed but enter times not match\n", obj->userId_, obj->roleId_);
                            obj->gwHeartbeatFailCount_++;
                        }
                        
                        break;
                    }
                    case 1: {
                        obj->gwHeartbeatFailCount_ = 0;
                        break;
                    }
                }

                // 这里不需要判断curEnterTimes == obj->_enterTimes是因为_enterTimes被改变的时候_gwHeartbeatFailCount被设置为0
                if (/*curEnterTimes == obj->enterTimes_ && */obj->gwHeartbeatFailCount_ >= 3) {
                    WARN_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to gw failed\n", obj->userId_, obj->roleId_);

                    obj->gatewayServerStub_ = nullptr;
                    obj->onOffline(); // 通知上层玩家离线
                }
            }
        }

        if (success) {
            // 对记录对象进行心跳
            success = obj->heartbeatToRecord() == 1;

            if (!success) {
                WARN_LOG("GameObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to record failed\n", obj->userId_, obj->roleId_);
            }
        }

        // 若设置超时不成功，销毁游戏对象
        if (!success) {
            if (obj->running_) {
                obj->leaveGame();
                assert(obj->running_ == false);
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

    while (obj->running_) {
        obj->cond_.wait(SYNC_PERIOD);

        if (!obj->running_) {
            break;
        }

        // 向记录服同步数据（销毁前也应该将脏数据同步给记录服）
        obj->buildSyncDatas(syncDatas, removes);
        if (!syncDatas.empty() || !removes.empty()) {
            obj->sync(syncDatas, removes);
            syncDatas.clear();
            removes.clear();
        }

    }

    return nullptr;
}

void *GameObject::updateRoutine(void *arg) {
    GameObjectRoutineArg* routineArg = (GameObjectRoutineArg*)arg;
    std::shared_ptr<GameObject> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;

    while (obj->running_) {
        obj->cond_.wait(g_GameCenter.getGameObjectUpdatePeriod());

        if (!obj->running_) {
            // 游戏对象已被销毁
            break;
        }

        gettimeofday(&t, NULL);
        obj->update(t);
    }

    return nullptr;
}
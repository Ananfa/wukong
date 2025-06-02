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

#include "lobby_object.h"

#include "corpc_routine_env.h"
#include "lobby_config.h"
#include "lobby_object_manager.h"
#include "message_handle_manager.h"
#include "redis_pool.h"
#include "redis_utils.h"
#include "proto_utils.h"
#include "agent_manager.h"
#include "gateway_agent.h"
#include "record_agent.h"
#include "share/const.h"

using namespace wukong;

LobbyObject::~LobbyObject() {}

void LobbyObject::setGatewayServerId(ServerId sid) {
    gatewayId_ = sid;
}

void LobbyObject::setRecordServerId(ServerId sid) {
    recordId_ = sid;
}

void LobbyObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) {
    data_->buildSyncDatas(datas, removes);
}

void LobbyObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
    data_->buildAllDatas(datas);
}

void LobbyObject::start() {
    running_ = true;

    RoutineEnvironment::startCoroutine(heartbeatRoutine, this);

    // 启动定期存盘协程（每隔一段时间将脏数据同步给记录服，协程中退出循环时也会执行一次）
    RoutineEnvironment::startCoroutine(syncRoutine, this);

    // 根据配置启动周期处理协程（执行周期更新逻辑）
    if (g_LobbyConfig.getUpdatePeriod() > 0) {
        RoutineEnvironment::startCoroutine(updateRoutine, this);
    }

    onStart();
}

void LobbyObject::stop() {
    if (running_) {
        DEBUG_LOG("LobbyObject::stop user[%llu] role[%llu] token:%s\n", userId_, roleId_, lToken_.c_str());
        running_ = false;

        // 若不清emiter，会导致shared_ptr循环引用问题
        emiter_.clear();

        onDestory();

        cond_.broadcast();

        // TODO: 在这里直接进行redis操作会有协程切换，导致一些流程同步问题，需要考虑一下是否需要换地方调用
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("LobbyObject::stop -- user[%llu] role[%llu] connect to cache failed\n", userId_, roleId_);
            return;
        }

        if (RedisUtils::RemoveLobbyAddress(cache, roleId_, lToken_) == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("LobbyObject::stop -- user[%llu] role[%llu] remove location failed", userId_, roleId_);
            return;
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
    }
}

void LobbyObject::enterGame() {
    gwHeartbeatFailCount_ = 0; // 重置心跳
    enterTimes_++;
    onEnterGame();
}

void LobbyObject::leaveGame() {
    g_LobbyObjectManager.leaveGame(roleId_);
}

void LobbyObject::regEventHandle(const std::string &name, EventHandle handle) {
    emiter_.regEventHandle(name, handle);
}

void LobbyObject::fireEvent(const Event &event) {
    emiter_.fireEvent(event);
}

void LobbyObject::send(int32_t type, uint16_t tag, const std::string &msg) {
    if (gatewayId_ == 0) {
        ERROR_LOG("LobbyObject::send -- user[%llu] role[%llu] type %d gateway not set\n", userId_, roleId_, type);
        return;
    }

    GatewayAgent *gateAgent = (GatewayAgent*)g_AgentManager.getAgent(SERVER_TYPE_GATE);

    gateAgent->send(gatewayId_, type, tag, userId_, lToken_, msg);
}

void LobbyObject::send(int32_t type, uint16_t tag, google::protobuf::Message &msg) {
    if (gatewayId_ == 0) {
        ERROR_LOG("LobbyObject::send -- user[%llu] role[%llu] type %d gateway not set\n", userId_, roleId_, type);
        return;
    }

    std::string bufStr(msg.ByteSizeLong(), 0);
    uint8_t *buf = (uint8_t *)bufStr.data();
    msg.SerializeWithCachedSizesToArray(buf);

    send(type, tag, bufStr);
}

//int LobbyObject::reportLobbyObjectPos() {
//    if (!gatewayServerStub_) {
//        ERROR_LOG("LobbyObject::setLobbyObjectPos -- user[%llu] role[%llu] gateway stub not set\n", userId_, roleId_);
//        return -1;
//    }
//
//    pb::SetLobbyObjectPosRequest *request = new pb::SetLobbyObjectPosRequest();
//    pb::BoolValue *response = new pb::BoolValue();
//    Controller *controller = new Controller();
//    request->set_serverid(gatewayId_);
//    request->set_userid(userId_);
//    request->set_roleid(roleId_);
//    request->set_ltoken(lToken_);
//    request->set_gstype(g_GameCenter.getType());
//    request->set_gsid(manager_->getId());
//    gatewayServerStub_->setLobbyObjectPos(controller, request, response, nullptr);
//    
//    int ret;
//    if (controller->Failed()) {
//        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
//        ret = -2;
//    } else {
//        ret = response->value()?1:0;
//    }
//    
//    delete controller;
//    delete response;
//    delete request;
//
//    return ret;
//}

int LobbyObject::heartbeatToGateway() {
    assert(gatewayId_ != 0);
    GatewayAgent *gateAgent = (GatewayAgent*)g_AgentManager.getAgent(SERVER_TYPE_GATE);
    assert(gateAgent != nullptr);

    return gateAgent->heartbeat(gatewayId_, userId_, lToken_);
}

int LobbyObject::heartbeatToRecord() {
    assert(recordId_ != 0);
    RecordAgent *recordAgent = (RecordAgent*)g_AgentManager.getAgent(SERVER_TYPE_RECORD);
    assert(recordAgent != nullptr);

    return recordAgent->heartbeat(recordId_, roleId_, lToken_);
}

bool LobbyObject::sync(const std::list<std::pair<std::string, std::string>> &datas, const std::list<std::string> &removes) {
    assert(recordId_ != 0);
    RecordAgent *recordAgent = (RecordAgent*)g_AgentManager.getAgent(SERVER_TYPE_RECORD);
    assert(recordAgent != nullptr);

    return recordAgent->sync(recordId_, roleId_, lToken_, datas, removes);
}

void *LobbyObject::heartbeatRoutine( void *arg ) {
    std::shared_ptr<LobbyObject> obj = std::static_pointer_cast<LobbyObject>(((LobbyObject *)arg)->getPtr());

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
            ERROR_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] connect to cache failed\n", obj->userId_, obj->roleId_);

            success = false;
        } else {
            switch (RedisUtils::SetLobbyAddressTTL(cache, obj->roleId_, obj->lToken_)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCoreCache()->put(cache, true);
                    ERROR_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] check session failed for db error\n", obj->userId_, obj->roleId_);
                    success = false;
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    ERROR_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] check session failed\n", obj->userId_, obj->roleId_);
                    success = false;
                }
                case REDIS_SUCCESS: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    success = true;
                }
            }
        }

        // 注意: 对gateway对象进行心跳改为失败时发通知不直接删除lobby对象，交由上层决定是否删除lobby对象
        if (success) {
            if (obj->gatewayId_ != 0) {
                // 对网关对象进行心跳
                // 三次心跳不到gateway对象才销毁
                int curEnterTimes = obj->enterTimes_;
                switch (obj->heartbeatToGateway()) {
                    case -2: {// rpc出错（心跳超时）
                        obj->gwHeartbeatFailCount_++;
                        WARN_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat timeout, count:%d\n", obj->userId_, obj->roleId_, obj->gwHeartbeatFailCount_);
                        break;
                    }
                    case -1: {// gateway连不上
                        obj->gwHeartbeatFailCount_++;
                        WARN_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] gateway stub not found, count:%d\n", obj->userId_, obj->roleId_, obj->gwHeartbeatFailCount_);
                        break;
                    }
                    case 0: {// gateway对象不存在
                        // 注意：
                        // 这里有个问题：重登时正好lobbyobj向gatewayobj心跳，在gatewayobj的旧对象销毁新对象重建过程中刚好心跳请求到达，因此返回心跳失败--找不到gatewayobj，
                        // 在心跳RPC结果返回过程中，新gatewayobj创建完成并且通知lobbyobj连接建立，lobbyobj通知客户端进入游戏完成，（此处进行心跳失败处理将gateway对象stub清除了），
                        // 导致lobbyobj错误的丢掉了gatewayobj的连接。
                        // 解决方法：lobbyobj中增加一个登录计数值，心跳前和心跳后的登录计数值一样才是真的断线
                        if (curEnterTimes == obj->enterTimes_) {
                            WARN_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to gw failed for gw object not exit\n", obj->userId_, obj->roleId_);

                            obj->gatewayId_ = 0;
                            obj->onOffline(); // 通知上层玩家离线
                        } else {
                            WARN_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to gw failed but enter times not match\n", obj->userId_, obj->roleId_);
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
                    WARN_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to gw failed\n", obj->userId_, obj->roleId_);

                    obj->gatewayId_ = 0;
                    obj->onOffline(); // 通知上层玩家离线
                }
            }
        }

        if (success) {
            // 对记录对象进行心跳
            success = obj->heartbeatToRecord() == 1;

            if (!success) {
                WARN_LOG("LobbyObject::heartbeatRoutine -- user[%llu] role[%llu] heartbeat to record failed\n", obj->userId_, obj->roleId_);
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

void *LobbyObject::syncRoutine(void *arg) {
    std::shared_ptr<LobbyObject> obj = std::static_pointer_cast<LobbyObject>(((LobbyObject *)arg)->getPtr());

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

void *LobbyObject::updateRoutine(void *arg) {
    std::shared_ptr<LobbyObject> obj = std::static_pointer_cast<LobbyObject>(((LobbyObject *)arg)->getPtr());

    struct timeval t;

    while (obj->running_) {
        obj->cond_.wait(g_LobbyConfig.getUpdatePeriod());

        if (!obj->running_) {
            // 游戏对象已被销毁
            break;
        }

        gettimeofday(&t, NULL);
        obj->onUpdate(t);
    }

    return nullptr;
}

// 注意: 根据项目需求实现玩家对象的onUpdate方法
void LobbyObject::onUpdate(timeval now) {
    // 【测试代码】：测试发事件（每秒发一次事件）
    uint32_t testValue = 1;
    Event e("test_local_event");
    e.setParam("testValue", testValue);
    fireEvent(e);

    //std::string data = "hello world";
    //Event ge("test_global_event");
    //ge.setParam("data", data);
    //fireGlobalEvent(ge);
}

// 注意: 根据项目需求实现玩家对象的onUpdate方法
void LobbyObject::onStart() {
    // TODO: 这里应该初始化游戏对象中的各种组件模块，绑定各模块的事件处理

    // 【测试代码】：事件处理测试（以下是两种注册事件处理的方式，第一种是用bind，第二种是用lamda）
    regEventHandle("test_local_event", std::bind(&LobbyObject::onTestLocalEvent, std::static_pointer_cast<LobbyObject>(shared_from_this()), std::placeholders::_1));

    regEventHandle("test_local_event", [self = std::static_pointer_cast<LobbyObject>(shared_from_this())](const Event &e) {
        uint32_t testValue;
        e.getParam("testValue", testValue);
        self->onTestLocalEvent1(testValue);
    });

}

void LobbyObject::onDestory() {
    // 若不清timer，会导致shared_ptr循环引用问题
    if (leaveGameTimer_) {
        leaveGameTimer_->stop();
        leaveGameTimer_ = nullptr;
    }

    // TODO: 各种模块销毁
}

void LobbyObject::onEnterGame() { // 重登了
    std::list<std::pair<std::string, std::string>> datas;
    buildAllDatas(datas);
    std::string msgData = ProtoUtils::marshalDataFragments(datas);
    send(wukong::S2C_MESSAGE_ID_ENTERGAME, 0, msgData);

    // 取消离开游戏计时器
    if (leaveGameTimer_) {
        //DEBUG_LOG("LobbyObject::onEnterGame -- user[%llu] role[%llu] cancel leave-game-timer:%llu\n", _userId, _roleId, leaveGameTimer_.get());
        leaveGameTimer_->stop();
        leaveGameTimer_ = nullptr;
    }

    // TODO: 其他进入游戏逻辑

    // 发送进入大厅消息给客户端
    wukong::pb::Int32Value resp;
    resp.set_value(1);
    send(S2C_MESSAGE_ID_ENTERLOBBY, 0, resp);
}

void LobbyObject::onOffline() { // 断线了
    // 离线处理（这里通过timer等待5秒后离开游戏，5秒内如果重新登录了就不离开游戏了）
    leaveGameTimer_ = Timer::create(5000, [self = std::static_pointer_cast<LobbyObject>(shared_from_this())]() {
        self->leaveGame();
    });
    //DEBUG_LOG("LobbyObject::onOffline -- user[%llu] role[%llu] start leave-game-timer:%llu\n", _userId, _roleId, leaveGameTimer_.get());
    
    // TODO: 其他离线逻辑
}

// 【测试代码】：
void LobbyObject::onTestLocalEvent(const Event &e) {
    uint32_t testValue;
    e.getParam("testValue", testValue);
    DEBUG_LOG("LobbyObject::onTestLocalEvent, value:%d\n", testValue);

    wukong::pb::StringValue resp;
    resp.set_value("hello");
    send(S2C_MESSAGE_ID_ECHO, 0, resp);
}

void LobbyObject::onTestLocalEvent1(uint32_t testValue) {
    DEBUG_LOG("LobbyObject::onTestLocalEvent1, value:%d\n", testValue);
}

//void LobbyObject::onTestGlobalEvent(const Event &e) {
//    std::string data;
//    e.getParam("data", data);
//    DEBUG_LOG("LobbyObject::onTestGlobalEvent, data:%s\n", data.c_str());
//}

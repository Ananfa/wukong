#if 0

#include "lobby_game_object.h"
#include "demo_const.h"

using namespace demo;
using namespace corpc;

LobbyGameObject::LobbyGameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager): DemoGameObject(userId, roleId, serverId, lToken, manager) {
//DEBUG_LOG("LobbyGameObject::LobbyGameObject()\n");
}

LobbyGameObject::~LobbyGameObject() {
//DEBUG_LOG("LobbyGameObject::~LobbyGameObject()\n");
}

void LobbyGameObject::update(timeval now) {
    // 【测试代码】：测试发事件（每秒发一次事件）
    uint32_t testValue = 1;
    Event e("test_local_event");
    e.setParam("testValue", testValue);
    fireLocalEvent(e);

    std::string data = "hello world";
    Event ge("test_global_event");
    ge.setParam("data", data);
    fireGlobalEvent(ge);
}

void LobbyGameObject::onStart() {
    // TODO: 这里应该初始化游戏对象中的各种组件模块，绑定各模块的事件处理

    // 【测试代码】：事件处理测试（以下是两种注册事件处理的方式，第一种是用bind，第二种是用lamda）
    regLocalEventHandle("test_local_event", std::bind(&LobbyGameObject::onTestLocalEvent, std::static_pointer_cast<LobbyGameObject>(shared_from_this()), std::placeholders::_1));

    regLocalEventHandle("test_local_event", [self = std::static_pointer_cast<LobbyGameObject>(shared_from_this())](const Event &e) {
        uint32_t testValue;
        e.getParam("testValue", testValue);
        self->onTestLocalEvent1(testValue);
    });

    regGlobalEventHandle("test_global_event", std::bind(&LobbyGameObject::onTestGlobalEvent, std::static_pointer_cast<LobbyGameObject>(shared_from_this()), std::placeholders::_1));
}

void LobbyGameObject::onDestory() {
    // 若不清timer，会导致shared_ptr循环引用问题
    if (leaveGameTimer_) {
        leaveGameTimer_->stop();
        leaveGameTimer_ = nullptr;
    }

    // TODO: 各种模块销毁
}

void LobbyGameObject::onEnterGame() { // 重登了
    DemoGameObject::onEnterGame();

    // 取消离开游戏计时器
    if (leaveGameTimer_) {
        //DEBUG_LOG("LobbyGameObject::onEnterGame -- user[%llu] role[%llu] cancel leave-game-timer:%llu\n", _userId, _roleId, leaveGameTimer_.get());
        leaveGameTimer_->stop();
        leaveGameTimer_ = nullptr;
    }

    // TODO: 其他进入游戏逻辑

    // 发送进入大厅消息给客户端
    wukong::pb::Int32Value resp;
    resp.set_value(1);
    send(S2C_MESSAGE_ID_ENTERLOBBY, 0, resp);
}

void LobbyGameObject::onOffline() { // 断线了
    // 离线处理（这里通过timer等待5秒后离开游戏，5秒内如果重新登录了就不离开游戏了）
    leaveGameTimer_ = Timer::create(5000, [self = shared_from_this()]() {
        self->leaveGame();
    });
    //DEBUG_LOG("LobbyGameObject::onOffline -- user[%llu] role[%llu] start leave-game-timer:%llu\n", _userId, _roleId, leaveGameTimer_.get());
    
    // TODO: 其他离线逻辑
}

// 【测试代码】：
void LobbyGameObject::onTestLocalEvent(const Event &e) {
    uint32_t testValue;
    e.getParam("testValue", testValue);
    DEBUG_LOG("LobbyGameObject::onTestLocalEvent, value:%d\n", testValue);

    wukong::pb::StringValue resp;
    resp.set_value("hello");
    send(S2C_MESSAGE_ID_ECHO, 0, resp);
}

void LobbyGameObject::onTestLocalEvent1(uint32_t testValue) {
    DEBUG_LOG("LobbyGameObject::onTestLocalEvent1, value:%d\n", testValue);
}

void LobbyGameObject::onTestGlobalEvent(const Event &e) {
    std::string data;
    e.getParam("data", data);
    DEBUG_LOG("LobbyGameObject::onTestGlobalEvent, data:%s\n", data.c_str());
}

#endif
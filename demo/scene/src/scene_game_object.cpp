#include "scene_game_object.h"

using namespace demo;
using namespace corpc;

SceneGameObject::SceneGameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager): DemoGameObject(userId, roleId, serverId, lToken, manager) {
//DEBUG_LOG("SceneGameObject::SceneGameObject()\n");
}

SceneGameObject::~SceneGameObject() {
//DEBUG_LOG("SceneGameObject::~SceneGameObject()\n");
}

void SceneGameObject::update(timeval now) {
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

void SceneGameObject::onStart() {
    // TODO: 这里应该初始化游戏对象中的各种组件模块，绑定各模块的事件处理

    // 【测试代码】：事件处理测试（以下是两种注册事件处理的方式，第一种是用bind，第二种是用lamda）
    regLocalEventHandle("test_local_event", std::bind(&SceneGameObject::onTestLocalEvent, std::static_pointer_cast<SceneGameObject>(shared_from_this()), std::placeholders::_1));

    regLocalEventHandle("test_local_event", [self = std::static_pointer_cast<SceneGameObject>(shared_from_this())](const Event &e) {
        uint32_t testValue;
        e.getParam("testValue", testValue);
        self->onTestLocalEvent1(testValue);
    });

    regGlobalEventHandle("test_global_event", std::bind(&SceneGameObject::onTestGlobalEvent, std::static_pointer_cast<SceneGameObject>(shared_from_this()), std::placeholders::_1));
}

void SceneGameObject::onDestory() {
    // 若不清timer，会导致shared_ptr循环引用问题
    if (_leaveGameTimer) {
        _leaveGameTimer->stop();
        _leaveGameTimer = nullptr;
    }

    // TODO: 各种模块销毁
}

void SceneGameObject::onEnterGame() { // 重登了
    DemoGameObject::onEnterGame();

    // 取消离开游戏计时器
    if (_leaveGameTimer) {
        //DEBUG_LOG("SceneGameObject::onEnterGame -- user[%llu] role[%llu] cancel leave-game-timer:%llu\n", _userId, _roleId, _leaveGameTimer.get());
        _leaveGameTimer->stop();
        _leaveGameTimer = nullptr;
    }

    // TODO: 其他进入游戏逻辑
}

void SceneGameObject::onOffline() { // 断线了
    // 离线处理（这里通过timer等待5秒后离开游戏，5秒内如果重新登录了就不离开游戏了）
    // 注意：对于W3L类型游戏，处理方式应该是要等到队伍所有玩家都离线时才销毁
    _leaveGameTimer = Timer::create(5000, [self = shared_from_this()]() {
        self->leaveGame();
    });
    //DEBUG_LOG("SceneGameObject::onOffline -- user[%llu] role[%llu] start leave-game-timer:%llu\n", _userId, _roleId, _leaveGameTimer.get());
    
    // TODO: 其他离线逻辑
}

// 【测试代码】：
void SceneGameObject::onTestLocalEvent(const Event &e) {
    uint32_t testValue;
    e.getParam("testValue", testValue);
    ERROR_LOG("SceneGameObject::onTestLocalEvent, value:%d\n", testValue);
}

void SceneGameObject::onTestLocalEvent1(uint32_t testValue) {
    ERROR_LOG("SceneGameObject::onTestLocalEvent1, value:%d\n", testValue);
}

void SceneGameObject::onTestGlobalEvent(const Event &e) {
    std::string data;
    e.getParam("data", data);
    ERROR_LOG("SceneGameObject::onTestGlobalEvent, data:%s\n", data.c_str());
}

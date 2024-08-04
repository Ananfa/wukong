#include "my_lobby_object.h"
#include "demo_const.h"

using namespace demo;
using namespace corpc;

MyLobbyObject::MyLobbyObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken): DemoLobbyObject(userId, roleId, serverId, lToken) {

}

MyLobbyObject::~MyLobbyObject() {

}

void MyLobbyObject::update(timeval now) {
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

void MyLobbyObject::onStart() {
    // TODO: 这里应该初始化游戏对象中的各种组件模块，绑定各模块的事件处理

    // 【测试代码】：事件处理测试（以下是两种注册事件处理的方式，第一种是用bind，第二种是用lamda）
    regEventHandle("test_local_event", std::bind(&MyLobbyObject::onTestLocalEvent, std::static_pointer_cast<MyLobbyObject>(shared_from_this()), std::placeholders::_1));

    regEventHandle("test_local_event", [self = std::static_pointer_cast<MyLobbyObject>(shared_from_this())](const Event &e) {
        uint32_t testValue;
        e.getParam("testValue", testValue);
        self->onTestLocalEvent1(testValue);
    });

}

void MyLobbyObject::onDestory() {
    // 若不清timer，会导致shared_ptr循环引用问题
    if (leaveGameTimer_) {
        leaveGameTimer_->stop();
        leaveGameTimer_ = nullptr;
    }

    // TODO: 各种模块销毁
}

void MyLobbyObject::onEnterGame() { // 重登了
    DemoLobbyObject::onEnterGame();

    // 取消离开游戏计时器
    if (leaveGameTimer_) {
        //DEBUG_LOG("MyLobbyObject::onEnterGame -- user[%llu] role[%llu] cancel leave-game-timer:%llu\n", _userId, _roleId, leaveGameTimer_.get());
        leaveGameTimer_->stop();
        leaveGameTimer_ = nullptr;
    }

    // TODO: 其他进入游戏逻辑

    // 发送进入大厅消息给客户端
    wukong::pb::Int32Value resp;
    resp.set_value(1);
    send(S2C_MESSAGE_ID_ENTERLOBBY, 0, resp);
}

void MyLobbyObject::onOffline() { // 断线了
    // 离线处理（这里通过timer等待5秒后离开游戏，5秒内如果重新登录了就不离开游戏了）
    leaveGameTimer_ = Timer::create(5000, [self = std::static_pointer_cast<MyLobbyObject>(shared_from_this())]() {
        self->leaveGame();
    });
    //DEBUG_LOG("MyLobbyObject::onOffline -- user[%llu] role[%llu] start leave-game-timer:%llu\n", _userId, _roleId, leaveGameTimer_.get());
    
    // TODO: 其他离线逻辑
}

// 【测试代码】：
void MyLobbyObject::onTestLocalEvent(const Event &e) {
    uint32_t testValue;
    e.getParam("testValue", testValue);
    DEBUG_LOG("MyLobbyObject::onTestLocalEvent, value:%d\n", testValue);

    wukong::pb::StringValue resp;
    resp.set_value("hello");
    send(S2C_MESSAGE_ID_ECHO, 0, resp);
}

void MyLobbyObject::onTestLocalEvent1(uint32_t testValue) {
    DEBUG_LOG("MyLobbyObject::onTestLocalEvent1, value:%d\n", testValue);
}

//void MyLobbyObject::onTestGlobalEvent(const Event &e) {
//    std::string data;
//    e.getParam("data", data);
//    DEBUG_LOG("MyLobbyObject::onTestGlobalEvent, data:%s\n", data.c_str());
//}

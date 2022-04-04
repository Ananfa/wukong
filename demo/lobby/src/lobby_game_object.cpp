#include "lobby_game_object.h"

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
    uint32_t lv = 1;
    Event e("level_up");
    e.setParam("lv", lv);
    _emiter.fireEvent(e);
}

void LobbyGameObject::onStart() {
    // TODO: 这里应该初始化游戏对象中的各种组件模块，绑定各模块的事件处理

    // 【测试代码】：事件处理测试
    _emiter.addEventHandle("level_up", [obj = shared_from_this()](const Event &e) {
        std::shared_ptr<LobbyGameObject> self = std::static_pointer_cast<LobbyGameObject>(obj);
        uint32_t lv;
        e.getParam("lv", lv);
        self->onLevelUp(e);
    });
}

void LobbyGameObject::onDestory() {
    // 若不清emiter，会导致shared_ptr循环引用问题
    _emiter.clear();

    // 若不清timer，会导致shared_ptr循环引用问题
    if (_leaveGameTimer) {
        _leaveGameTimer->stop();
        _leaveGameTimer = nullptr;
    }

    // TODO: 各种模块销毁
}

void LobbyGameObject::onEnterGame() { // 重登了
    DemoGameObject::onEnterGame();

    // 取消离开游戏计时器
    if (_leaveGameTimer) {
        //DEBUG_LOG("LobbyGameObject::onEnterGame -- user[%llu] role[%llu] cancel leave-game-timer:%llu\n", _userId, _roleId, _leaveGameTimer.get());
        _leaveGameTimer->stop();
        _leaveGameTimer = nullptr;
    }

    // TODO: 其他进入游戏逻辑
}

void LobbyGameObject::onOffline() { // 断线了
    // 离线处理（这里通过timer等待5秒后离开游戏，5秒内如果重新登录了就不离开游戏了）
    _leaveGameTimer = Timer::create(5000, [self = shared_from_this()]() {
        self->leaveGame();
    });
    //DEBUG_LOG("LobbyGameObject::onOffline -- user[%llu] role[%llu] start leave-game-timer:%llu\n", _userId, _roleId, _leaveGameTimer.get());
    
    // TODO: 其他离线逻辑
}

// 【测试代码】：
void LobbyGameObject::onLevelUp(const Event &e) {
    DEBUG_LOG("LobbyGameObject::onLevelUp\n");
}
#include "lobby_game_object.h"

using namespace demo;
using namespace corpc;

LobbyGameObject::LobbyGameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager): DemoGameObject(userId, roleId, serverId, lToken, manager) {

}

LobbyGameObject::~LobbyGameObject() {

}

void LobbyGameObject::update(uint64_t nowSec) {
    // TODO:
}

void LobbyGameObject::onEnterGame() { // 重登了
    DemoGameObject::onEnterGame();

    // 取消离开游戏计时器
    if (_leaveGameTimer) {
        ERROR_LOG("LobbyGameObject::onEnterGame -- user[%llu] role[%llu] cancel leave-game-timer:%llu\n", _userId, _roleId, _leaveGameTimer.get());
        _leaveGameTimer->stop();
        _leaveGameTimer = nullptr;
    }

    // TODO: 其他逻辑
}

void LobbyGameObject::onOffline() { // 断线了
    DemoGameObject::onOffline();

    // 离线处理（这里通过timer等待5秒后离开游戏，5秒内如果重新登录了就不离开游戏了）
    _leaveGameTimer = Timer::create(5000, [self = shared_from_this()]() {
        self->leaveGame();
    });
    ERROR_LOG("LobbyGameObject::onOffline -- user[%llu] role[%llu] start leave-game-timer:%llu\n", _userId, _roleId, _leaveGameTimer.get());
    
}
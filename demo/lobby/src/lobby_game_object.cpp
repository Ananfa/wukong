#include "lobby_game_object.h"

using namespace demo;

LobbyGameObject::LobbyGameObject(UserId userId, RoleId roleId, ServerId serverId, uint32_t lToken, GameObjectManager *manager): DemoGameObject(userId, roleId, serverId, lToken, manager) {

}

LobbyGameObject::~LobbyGameObject() {

}

void LobbyGameObject::update(uint64_t nowSec) {
    // TODO:
}

void LobbyGameObject::onEnterGame() {
    DemoGameObject::onEnterGame();

    // TODO: 其他逻辑
}
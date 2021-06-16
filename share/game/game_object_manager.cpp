/*
 * Created by Xianke Liu on 2021/1/15.
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
#include "game_object_manager.h"
#include "game_center.h"

#include <sys/time.h>

using namespace wukong;

void GameObjectManager::init() {
    
}

void GameObjectManager::shutdown() {
    if (_shutdown) {
        return;
    }

    _shutdown = true;

    for (auto &gameObj : _roleId2GameObjectMap) {
        gameObj.second->stop();
    }

    _roleId2GameObjectMap.clear();
}

size_t GameObjectManager::size() {
    return _roleId2GameObjectMap.size();
}

bool GameObjectManager::exist(RoleId roleId) {
    return _roleId2GameObjectMap.find(roleId) != _roleId2GameObjectMap.end();
}

std::shared_ptr<GameObject> GameObjectManager::getGameObject(RoleId roleId) {
    auto it = _roleId2GameObjectMap.find(roleId);
    if (it == _roleId2GameObjectMap.end()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<GameObject> GameObjectManager::create(UserId userId, RoleId roleId, ServerId serverId, uint32_t lToken, ServerId gatewayId, ServerId recordId, const std::string &data) {
    if (_shutdown) {
        WARN_LOG("GameObjectManager::create -- already shutdown\n");
        return nullptr;
    }

    if (_roleId2GameObjectMap.find(roleId) != _roleId2GameObjectMap.end()) {
        ERROR_LOG("GameObjectManager::create -- game object already exist\n");
        return nullptr;
    }

    if (!g_GameCenter.getCreateGameObjectHandle()) {
        ERROR_LOG("GameObjectManager::create -- not set CreateGameObjectHandle\n");
        return nullptr;
    }

    // 创建GameObject
    auto obj = g_GameCenter.getCreateGameObjectHandle()(userId, roleId, serverId, lToken, this, data);

    // 设置gateway和record stub
    if (!obj->setGatewayServerStub(gatewayId)) {
        ERROR_LOG("GameObjectManager::create -- can't set gateway stub\n");
        return nullptr;
    }

    if (!obj->setRecordServerStub(recordId)) {
        ERROR_LOG("GameObjectManager::create -- can't set record stub\n");
        return nullptr;
    }

    _roleId2GameObjectMap.insert(std::make_pair(roleId, obj));
    obj->start();

    return obj;
}

bool GameObjectManager::remove(RoleId roleId) {
    auto it = _roleId2GameObjectMap.find(roleId);
    if (it == _roleId2GameObjectMap.end()) {
        return false;
    }

    it->second->stop();
    _roleId2GameObjectMap.erase(it);

    return true;
}
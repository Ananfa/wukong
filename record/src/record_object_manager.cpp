/*
 * Created by Xianke Liu on 2021/5/6.
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
#include "record_object_manager.h"
#include "record_delegate.h"

#include <sys/time.h>

using namespace wukong;

void RecordObjectManager::init() {
    
}

void RecordObjectManager::shutdown() {
    if (_shutdown) {
        return;
    }

    _shutdown = true;

    for (auto &recordObj : _roleId2RecordObjectMap) {
        recordObj.second->stop();
    }

    _roleId2RecordObjectMap.clear();
}

size_t RecordObjectManager::size() {
    return _roleId2RecordObjectMap.size();
}

bool RecordObjectManager::exist(RoleId roleId) {
    return _roleId2RecordObjectMap.find(roleId) != _roleId2RecordObjectMap.end();
}

std::shared_ptr<RecordObject> RecordObjectManager::getRecordObject(RoleId roleId) {
    auto it = _roleId2RecordObjectMap.find(roleId);
    if (it == _roleId2RecordObjectMap.end()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<RecordObject> RecordObjectManager::create(RoleId roleId, ServerId serverId, const std::string &rToken, std::list<std::pair<std::string, std::string>> &datas) {
    if (_shutdown) {
        WARN_LOG("RecordObjectManager::create -- already shutdown\n");
        return nullptr;
    }

    if (_roleId2RecordObjectMap.find(roleId) != _roleId2RecordObjectMap.end()) {
        ERROR_LOG("RecordObjectManager::create -- record object already exist\n");
        return nullptr;
    }

    if (!g_RecordDelegate.getCreateRecordObjectHandle()) {
        ERROR_LOG("RecordObjectManager::create -- not set CreateRecordObjectHandle\n");
        return nullptr;
    }

    // 创建RecordObject
    auto obj = g_RecordDelegate.getCreateRecordObjectHandle()(roleId, serverId, rToken, this, datas);

    _roleId2RecordObjectMap.insert(std::make_pair(roleId, obj));
    obj->start();

    return obj;
}

bool RecordObjectManager::remove(RoleId roleId) {
    auto it = _roleId2RecordObjectMap.find(roleId);
    if (it == _roleId2RecordObjectMap.end()) {
        return false;
    }

    it->second->stop();
    _roleId2RecordObjectMap.erase(it);
    return true;
}
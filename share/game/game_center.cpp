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

#include "game_center.h"

using namespace wukong;

void GameCenter::init(GameServerType stype, uint32_t gameObjectUpdatePeriod) {
    _type = stype;
    _gameObjectUpdatePeriod = gameObjectUpdatePeriod;

    _geventListener.init();
}

bool GameCenter::registerMessage(int msgType,
                                    google::protobuf::Message *proto,
                                    bool needCoroutine,
                                    MessageHandle handle) {
    if (_registerMessageMap.find(msgType) != _registerMessageMap.end()) {
        return false;
    }

    RegisterMessageInfo info;
    info.proto = proto;
    info.needCoroutine = needCoroutine;
    info.handle = handle;
    
    _registerMessageMap.insert(std::make_pair(msgType, info));
    
    return true;
}

void GameCenter::handleMessage(std::shared_ptr<GameObject> obj, int msgType, uint16_t tag, const std::string &rawMsg) {
    // TODO: 从lua环境中判断消息是否需要进行lua处理（这里新增的消息或者修正的消息都可以）

    // TODO: 如果有绑定的Lua消息处理，执行Lua消息处理（是否需要启动协程？既然无法判断要不要开协程就都开协程进行处理，反正与lua的性能相比协程切换那点损耗算不得什么）
    
    auto iter = _registerMessageMap.find(msgType);
    if (iter == _registerMessageMap.end()) {
        ERROR_LOG("GameCenter::handleMessage -- unknown message type: %d\n", msgType);
        return;
    }

    google::protobuf::Message *msg = nullptr;
    if (iter->second.proto) {
        msg = iter->second.proto->New();
        if (!msg->ParseFromString(rawMsg)) {
            // 出错处理
            ERROR_LOG("GameCenter::handleMessage -- parse fail for message: %d\n", msgType);
            delete msg;
            return;
        } else {
            assert(!rawMsg.empty());
        }
    }

    std::shared_ptr<google::protobuf::Message> targetMsg = std::shared_ptr<google::protobuf::Message>(msg);

    if (iter->second.needCoroutine) {
        HandleMessageInfo *info = new HandleMessageInfo();
        info->obj = obj;
        info->msg = targetMsg;
        info->tag = tag;
        info->handle = iter->second.handle;

        RoutineEnvironment::startCoroutine(handleMessageRoutine, info);
        return;
    }

    iter->second.handle(obj, tag, targetMsg);
}

void *GameCenter::handleMessageRoutine(void *arg) {
    HandleMessageInfo *info = (HandleMessageInfo *)arg;

    info->handle(info->obj, info->tag, info->msg);
    delete info;
    
    return nullptr;
}

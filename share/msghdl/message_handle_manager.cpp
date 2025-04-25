/*
 * Created by Xianke Liu on 2024/7/31.
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

#include "message_handle_manager.h"
#include "corpc_pubsub.h"
#include "redis_pool.h"
#include "redis_utils.h"
#include "rapidjson/document.h"

using namespace rapidjson;
using namespace wukong;

void MessageHandleManager::init() {
    //geventListener_.init();

    // 从Redis中获取初始的hotfix消息信息
    RoutineEnvironment::startCoroutine([](void * arg) -> void* {
        g_MessageHandleManager.resetHotfix();
        return NULL;
    }, NULL);

    // 用订阅主题“热更”，不需要用全服事件
    PubsubService::Subscribe("WK_Hotfix", true, [](const std::string& topic, const std::string& msg) {
        g_MessageHandleManager.resetHotfix();
    });
}

bool MessageHandleManager::registerMessage(int msgType,
                                    google::protobuf::Message *proto,
                                    bool needCoroutine,
                                    MessageHandle handle) {
    if (registerMessageMap_.find(msgType) != registerMessageMap_.end()) {
        return false;
    }

    RegisterMessageInfo info;
    info.proto = proto;
    info.needCoroutine = needCoroutine;
    info.handle = handle;
    info.needHotfix = false;
    
    registerMessageMap_.insert(std::make_pair(msgType, info));
    
    return true;
}

void MessageHandleManager::handleMessage(std::shared_ptr<MessageTarget> obj, int msgType, uint16_t tag, const std::string &rawMsg) {
    // 如果有绑定的Lua消息hotfix处理，执行Lua消息处理（是否需要启动协程？既然无法判断要不要开协程就都开协程进行处理，反正与lua的性能相比协程切换那点损耗算不得什么）
    //DEBUG_LOG("MessageHandleManager::handleMessage\n");
    auto iter = registerMessageMap_.find(msgType);
    if (iter == registerMessageMap_.end()) {
        ERROR_LOG("MessageHandleManager::handleMessage -- unknown message type: %d\n", msgType);
        return;
    }

    google::protobuf::Message *msg = nullptr;
    if (iter->second.proto) {
        msg = iter->second.proto->New();
        if (!msg->ParseFromString(rawMsg)) {
            // 出错处理
            ERROR_LOG("MessageHandleManager::handleMessage -- parse fail for message: %d\n", msgType);
            delete msg;
            return;
        } else {
            assert(!rawMsg.empty());
        }
    }

    std::shared_ptr<google::protobuf::Message> targetMsg = std::shared_ptr<google::protobuf::Message>(msg);

    obj->handleMessage(msgType, tag, targetMsg, iter->second.handle, iter->second.needCoroutine, iter->second.needHotfix);
}

void MessageHandleManager::resetHotfix() {
    // 从Redis中读取热更消息列表，并刷新注册消息信息中的热更标记
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("MessageHandleManager::resetHotfix -- connect to cache failed\n");
        return;
    }

    std::string hotfixData;
    switch (RedisUtils::GetHotfixData(cache, hotfixData)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("MessageHandleManager::resetHotfix -- get hotfix data failed for db error");
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("MessageHandleManager::resetHotfix -- get hotfix data failed for invalid data type\n");
            return;
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    Document doc;
    if (doc.Parse(hotfixData.c_str()).HasParseError()) {
        ERROR_LOG("MessageHandleManager::resetHotfix -- parse hotfix data failed\n");
        return;
    }

    if (!doc.IsArray()) {
        ERROR_LOG("MessageHandleManager::resetHotfix -- parse hotfix data failed for invalid type\n");
        return;
    }

    std::map<int, bool> hotfixMap;
    for (SizeType i = 0; i < doc.Size(); i++) {
        int msgType = doc[i].GetInt();
        if (hotfixMap.find(msgType) == hotfixMap.end()) { // 防重
            hotfixMap.insert(std::make_pair(msgType, true));
        }
    }

    for (auto &kv : registerMessageMap_) {
        if (hotfixMap.find(kv.first) != hotfixMap.end()) {
            if (!kv.second.needHotfix) {
                ERROR_LOG("MessageHandleManager::resetHotfix -- msg:%d\n", kv.first);
                kv.second.needHotfix = true;
            }
        } else {
            if (kv.second.needHotfix) {
                kv.second.needHotfix = false;
            }
        }
    }
}

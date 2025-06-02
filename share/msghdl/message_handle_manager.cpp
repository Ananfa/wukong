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
//#include "corpc_pubsub.h"
//#include "redis_pool.h"
//#include "redis_utils.h"
//#include "rapidjson/document.h"
//#include "utility.h"

//#include <sstream>

//using namespace rapidjson;
using namespace wukong;

//void MessageHandleManager::init() {
//    // 从Redis中获取初始的hotfix消息信息
//    RoutineEnvironment::startCoroutine([](void * arg) -> void* {
//        MessageHandleManager* manager = static_cast<MessageHandleManager*>(arg);
//        manager->resetHotfix();
//        return NULL;
//    }, this);
//
//    // 用订阅主题“热更”
//    PubsubService::Subscribe("WK_Hotfix", true, [this](const std::string& topic, const std::string& msg) {
//        resetHotfix();
//    });
//
//    RoutineEnvironment::startCoroutine(updateRoutine, this);
//}

bool MessageHandleManager::registerMessage(int32_t msgType, google::protobuf::Message *proto, bool needCoroutine, MessageHandle handle) {
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

bool MessageHandleManager::isRegistedMessage(int32_t msgType) {
    return registerMessageMap_.find(msgType) != registerMessageMap_.end();
}

bool MessageHandleManager::getMessageInfo(int32_t msgType, google::protobuf::Message *&proto, bool &needCoroutine, bool &needHotfix, MessageHandle &handle) {
    auto iter = registerMessageMap_.find(msgType);
    if (iter == registerMessageMap_.end()) {
        return false;
    }

    proto = iter->second.proto;
    needCoroutine = iter->second.needCoroutine;
    needHotfix = iter->second.needHotfix;
    handle = iter->second.handle;
    return true;
}

void MessageHandleManager::clearNeedHotfix() {
    for (auto &kv : registerMessageMap_) {
        kv.second.needHotfix = false;
    }
}

void MessageHandleManager::setNeedHotfix(int32_t msgType) {
    auto it = registerMessageMap_.find(msgType);
    if (it == registerMessageMap_.end()) {
        return;
    }

    it->second.needHotfix = true;
}

//void MessageHandleManager::resetHotfix() {
//    // 从Redis中读取热更消息列表，并刷新注册消息信息中的热更标记
//    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
//    if (!cache) {
//        ERROR_LOG("MessageHandleManager::resetHotfix -- connect to cache failed\n");
//        return;
//    }
//
//    std::string hotfixData;
//    switch (RedisUtils::GetHotfixData(cache, hotfixData)) {
//        case REDIS_DB_ERROR: {
//            g_RedisPoolManager.getCoreCache()->put(cache, true);
//            ERROR_LOG("MessageHandleManager::resetHotfix -- get hotfix data failed for db error");
//            return;
//        }
//        case REDIS_FAIL: {
//            g_RedisPoolManager.getCoreCache()->put(cache, true);
//            ERROR_LOG("MessageHandleManager::resetHotfix -- get hotfix data failed for invalid data type\n");
//            return;
//        }
//    }
//    g_RedisPoolManager.getCoreCache()->put(cache, false);
//
//    Document doc;
//    if (doc.Parse(hotfixData.c_str()).HasParseError()) {
//        ERROR_LOG("MessageHandleManager::resetHotfix -- parse hotfix data failed\n");
//        return;
//    }
//
//    if (!doc.IsArray()) {
//        ERROR_LOG("MessageHandleManager::resetHotfix -- parse hotfix data failed for invalid type\n");
//        return;
//    }
//
//    for (auto &kv : registerMessageMap_) {
//        kv.second.needHotfix = false;
//    }
//
//    // 加载Lua文件，约定hotfix消息处理的lua文件名为“fix_<消息号>.lua”，并放在相对路径hotfix中
//    hotfixMap_.clear();
//    for (SizeType i = 0; i < doc.Size(); i++) {
//        int msgType = doc[i].GetInt();
//
//        // 只更新自己相关的消息
//        auto it = registerMessageMap_.find(msgType);
//        if (it == registerMessageMap_.end()) {
//            continue;
//        }
//
//        it->second.needHotfix = true;
//        LOG("MessageHandleManager::resetHotfix -- msg:%d\n", msgType);
//
//        if (hotfixMap_.find(msgType) == hotfixMap_.end()) { // 防重
//            std::string content;
//            std::ostringstream oss;
//            oss << "hotfix/fix_" << msgType << ".lua";
//            std::string filename = oss.str();
//
//            Utility::loadFileToString(filename.c_str(), content);
//
//            hotfixMap_.insert(std::make_pair(msgType, std::move(content)));
//        }
//    }
//
//    hotfixVersion_++;
//    for (auto info : luaStates_) {
//        lua_close(info.L);
//    }
//    luaStates_.clear();
//}

//bool MessageHandleManager::getLuaStateInfo(LuaStateInfo &info) {
//    if (luaStates_.empty()) {
//        std::unique_ptr<lua_State, decltype(&lua_close)> L(luaL_newstate(), lua_close);
//        if (!L) {
//            ERROR_LOG("luaL_newstate failed\n");
//            return false;
//        }
//
//        luaL_requiref(L.get(), LUA_LOADLIBNAME, luaopen_package, 1);
//        lua_pop(L.get(), 1);
//
//        for (auto &pair : hotfixMap_) {
//            std::ostringstream oss;
//            oss << "hotfix/fix_" << pair.first << ".lua";
//            std::string filename = oss.str();
//
//            if (luaL_loadbuffer(L.get(), pair.second.c_str(), pair.second.size(), filename.c_str()) != LUA_OK) {
//                ERROR_LOG("Failed to load Lua script for msgType %d: %s\n", pair.first, lua_tostring(L.get(), -1));
//                lua_pop(L.get(), 1);
//                continue;
//                //return false;
//            }
//           
//            // 运行Lua文件
//            if(lua_pcall(L.get(),0,0,0) != LUA_OK) {
//                ERROR_LOG("lua_pcall error for msgType %d: %s\n", pair.first, lua_tostring(L.get(), -1));
//                lua_pop(L.get(), 1);
//                continue;
//                //return false;
//            }
//        }
//
//        info.L = L.release();
//        info.lastUsedAt = time(NULL);
//        info.version = hotfixVersion_;
//
//        return true;
//    }
//
//    info = luaStates_.back();
//    luaStates_.pop_back();
//    return true;
//}
//
//void MessageHandleManager::backLuaStateInfo(LuaStateInfo info) {
//    if (info.version != hotfixVersion_) {
//        lua_close(info.L);
//        return;
//    }
//
//    info.lastUsedAt = time(NULL);
//    luaStates_.push_back(info);
//}
//
//void *MessageHandleManager::updateRoutine(void * arg) {
//    MessageHandleManager *self = (MessageHandleManager *)arg;
//
//    // 每分钟清理一次1分钟内未被使用的lua状态
//    while (true) {
//        sleep(60);
//
//        time_t expire = time(NULL) - 60;
//        // 最少保留10个状态
//        while (self->luaStates_.size() > 10) {
//            auto &info = self->luaStates_.front();
//            if (info.lastUsedAt > expire) {
//                break;
//            }
//
//            lua_close(info.L);
//            self->luaStates_.pop_front();
//        }
//    }
//
//    return nullptr;
//}
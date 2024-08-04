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

extern "C"  
{  
    #include "lua.h"  
    #include "lauxlib.h"  
    #include "lualib.h"  
}

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

    // TODO: 判断是否需要用lua执行
    if (iter->second.needHotfix) {
        //if (iter->second.needCoroutine) {
            HotfixMessageInfo *info = new HotfixMessageInfo();
            info->msgType = msgType;
            info->obj = obj;
            info->msg = targetMsg;
            info->tag = tag;

            RoutineEnvironment::startCoroutine(hotfixMessageRoutine, info);
            return;
        //}

        //callHotfix(obj, msgType, tag, targetMsg);
    } else {
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
}

void *MessageHandleManager::handleMessageRoutine(void *arg) {
    HandleMessageInfo *info = (HandleMessageInfo *)arg;

    info->handle(info->obj, info->tag, info->msg);
    delete info;
    
    return nullptr;
}

void *MessageHandleManager::hotfixMessageRoutine(void *arg) {
    HotfixMessageInfo *info = (HotfixMessageInfo *)arg;

    callHotfix(info->obj, info->msgType, info->tag, info->msg);
    delete info;
    
    return nullptr;
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

void MessageHandleManager::callHotfix(std::shared_ptr<MessageTarget> obj, int msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    ERROR_LOG("MessageHandleManager::callHotfix -- 1\n");

    lua_State *L = luaL_newstate();
    if (L == NULL)  
    {
        ERROR_LOG("MessageHandleManager::callHotfix -- luaL_newstate failed\n");
        return;  
    }

    luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1);
    lua_pop(L, 1);

    // 加载Lua文件，约定hotfix消息处理的lua文件名为“fix_<消息号>.lua”，并放在相对路径hotfix中
    // TODO：由于本地文件读写不会产生协程切换，加载脚本文件会卡住当前线程运行，因此需要做个内存脚本缓存，当在缓存中找不到时才将脚本文件内容加载到缓存中
    //      另外脚本刷新时清理脚本缓存
    char luaFilePath[32];
    sprintf(luaFilePath,"hotfix/fix_%d.lua", msgType);
    int bRet = luaL_loadfile(L, luaFilePath);  
    if(bRet)
    {
        ERROR_LOG("MessageHandleManager::callHotfix -- load %s error: %d\n", luaFilePath, bRet);

        lua_close(L);
        return;  
    }  
   
    // 运行Lua文件  
    bRet = lua_pcall(L,0,0,0);  
    if(bRet)  
    {  
        const char *pErrorMsg = lua_tostring(L, -1);
        ERROR_LOG("MessageHandleManager::callHotfix -- lua_pcall ret:%d error: %s\n", bRet, pErrorMsg);

        lua_close(L);
        return;  
    }

    // 获取lua消息处理，约定的消息处理函数名为“fix_<消息号>”
    char luaFun[16];
    sprintf(luaFun,"fix_%d", msgType);
    lua_getglobal(L, luaFun);     // 获取函数，压入栈中  
    lua_pushnumber(L, (long)obj.get());          // 压入第一个参数  
    lua_pushnumber(L, (long)msg.get());          // 压入第二个参数  
    lua_pushnumber(L, tag);                      // 压入第三个参数 
    int iRet= lua_pcall(L, 3, 1, 0);// 调用函数，调用完成以后，会将返回值压入栈中，2表示参数个数，1表示返回结果个数。  
    if (iRet)                       // 调用出错  
    {
        const char *pErrorMsg = lua_tostring(L, -1);
        ERROR_LOG("MessageHandleManager::callHotfix -- call lua failed: %s\n", pErrorMsg);

        lua_close(L);
        return;  
    }

    // 关闭state
    lua_close(L);
}
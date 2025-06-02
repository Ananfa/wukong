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
#if 0
#include "game_center.h"
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

void GameCenter::init(GameServerType stype, uint32_t gameObjectUpdatePeriod) {
    type_ = stype;
    gameObjectUpdatePeriod_ = gameObjectUpdatePeriod;

    geventListener_.init();

    // 从Redis中获取初始的hotfix消息信息
    RoutineEnvironment::startCoroutine([](void * arg) -> void* {
        g_GameCenter.resetHotfix();
        return NULL;
    }, NULL);

    // 用订阅主题“热更”，不需要用全服事件
    PubsubService::Subscribe("WK_Hotfix", true, [](const std::string& topic, const std::string& msg) {
        g_GameCenter.resetHotfix();
    });
}

bool GameCenter::registerMessage(int msgType,
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

void GameCenter::handleMessage(std::shared_ptr<GameObject> obj, int msgType, uint16_t tag, const std::string &rawMsg) {
    // TODO: 从lua环境中判断消息是否需要进行lua处理（这里新增的消息或者修正的消息都可以）

    // TODO: 如果有绑定的Lua消息处理，执行Lua消息处理（是否需要启动协程？既然无法判断要不要开协程就都开协程进行处理，反正与lua的性能相比协程切换那点损耗算不得什么）

    //DEBUG_LOG("GameCenter::handleMessage\n");
    auto iter = registerMessageMap_.find(msgType);
    if (iter == registerMessageMap_.end()) {
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

void *GameCenter::handleMessageRoutine(void *arg) {
    HandleMessageInfo *info = (HandleMessageInfo *)arg;

    info->handle(info->obj, info->tag, info->msg);
    delete info;
    
    return nullptr;
}

void *GameCenter::hotfixMessageRoutine(void *arg) {
    HotfixMessageInfo *info = (HotfixMessageInfo *)arg;

    callHotfix(info->obj, info->msgType, info->tag, info->msg);
    delete info;
    
    return nullptr;
}

void GameCenter::resetHotfix() {
    // 从Redis中读取热更消息列表，并刷新注册消息信息中的热更标记
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("GameCenter::resetHotfix -- connect to cache failed\n");
        return;
    }

    std::string hotfixData;
    switch (RedisUtils::GetHotfixData(cache, hotfixData)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GameCenter::resetHotfix -- get hotfix data failed for db error");
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GameCenter::resetHotfix -- get hotfix data failed for invalid data type\n");
            return;
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    Document doc;
    if (doc.Parse(hotfixData.c_str()).HasParseError()) {
        ERROR_LOG("GameCenter::resetHotfix -- parse hotfix data failed\n");
        return;
    }

    if (!doc.IsArray()) {
        ERROR_LOG("GameCenter::resetHotfix -- parse hotfix data failed for invalid type\n");
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
                ERROR_LOG("GameCenter::resetHotfix -- msg:%d\n", kv.first);
                kv.second.needHotfix = true;
            }
        } else {
            if (kv.second.needHotfix) {
                kv.second.needHotfix = false;
            }
        }
    }
}

void GameCenter::callHotfix(std::shared_ptr<GameObject> obj, int msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    ERROR_LOG("GameCenter::callHotfix -- 1\n");

    lua_State *L = luaL_newstate();
    if (L == NULL)  
    {
        ERROR_LOG("GameCenter::callHotfix -- luaL_newstate failed\n");
        return;  
    }

    luaL_requiref(L, LUA_LOADLIBNAME, luaopen_package, 1);
    lua_pop(L, 1);

    // 加载Lua文件，约定hotfix消息处理的lua文件名为“fix_<消息号>.lua”，并放在相对路径hotfix中
    char luaFilePath[32];
    sprintf(luaFilePath,"hotfix/fix_%d.lua", msgType);
    int bRet = luaL_loadfile(L, luaFilePath);  
    if(bRet)
    {
        ERROR_LOG("GameCenter::callHotfix -- load %s error: %d\n", luaFilePath, bRet);

        lua_close(L);
        return;  
    }  
   
    // 运行Lua文件  
    bRet = lua_pcall(L,0,0,0);  
    if(bRet)  
    {  
        const char *pErrorMsg = lua_tostring(L, -1);
        ERROR_LOG("GameCenter::callHotfix -- lua_pcall ret:%d error: %s\n", bRet, pErrorMsg);

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
        ERROR_LOG("GameCenter::callHotfix -- call lua failed: %s\n", pErrorMsg);

        lua_close(L);
        return;  
    }

    // 关闭state
    lua_close(L);
}
#endif
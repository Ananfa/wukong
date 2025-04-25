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

#include "message_target.h"

#include "message_handle_manager.h"

extern "C"  
{  
    #include "lua.h"  
    #include "lauxlib.h"  
    #include "lualib.h"  
}

using namespace wukong;

MessageTarget::~MessageTarget() {}

void MessageTarget::handleMessage(int msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> &msg, MessageHandle handle, bool needCoroutine, bool needHotfix) {
    if (need_wait_ || wait_messages_.size() > 0) {
        wait_messages_.push_back({msgType, tag, msg, handle, needHotfix});
        return;
    }

    if (needCoroutine) {
        wait_messages_.push_back({msgType, tag, msg, handle, needHotfix});
        RoutineEnvironment::startCoroutine(handleMessageRoutine, this);
        return;
    }

    if (needHotfix) {
        callHotfix(msgType, tag, msg);
    } else {
        handle(getPtr(), tag, msg);
    }
}

void *MessageTarget::handleMessageRoutine(void *arg) {
    std::shared_ptr<MessageTarget> self = ((MessageTarget *)arg)->getPtr();

    self->needWait(true);

    do {
        auto &msgInfo = self->wait_messages_.front();

        if (msgInfo.needHotfix) {
            self->callHotfix(msgInfo.msgType, msgInfo.tag, msgInfo.targetMsg);
        } else {
            msgInfo.handle(self, msgInfo.tag, msgInfo.targetMsg);
        }

        self->wait_messages_.pop_front();
    } while (self->wait_messages_.size() > 0);

    self->needWait(false);

    return nullptr;
}

void MessageTarget::callHotfix(int msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
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
    lua_pushnumber(L, (long)this);          // 压入第一个参数  
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
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

#include <sstream>

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
    const char *scriptBuf = nullptr;
    size_t bufSize = 0;
    if (!g_MessageHandleManager.getHotfixScript(msgType, scriptBuf, bufSize)) {
        ERROR_LOG("hotfix script not found for msgType %d\n", msgType);
        return;
    }

    std::ostringstream oss;
    oss << "hotfix/fix_" << msgType << ".lua";
    std::string filename = oss.str();

    std::unique_ptr<lua_State, decltype(&lua_close)> L(luaL_newstate(), lua_close);
    if (!L) {
        ERROR_LOG("luaL_newstate failed\n");
        return;
    }

    luaL_requiref(L.get(), LUA_LOADLIBNAME, luaopen_package, 1);
    lua_pop(L.get(), 1);

    if (luaL_loadbuffer(L.get(), scriptBuf, bufSize, filename.c_str()) != LUA_OK) {
        ERROR_LOG("Failed to load Lua script for msgType %d: %s\n", msgType, lua_tostring(L.get(), -1));
        return;
    }
   
    // 运行Lua文件
    if(lua_pcall(L.get(),0,0,0) != LUA_OK) {
        ERROR_LOG("lua_pcall error for msgType %d: %s\n", msgType, lua_tostring(L.get(), -1));
        return;  
    }

    // 获取lua消息处理，约定的消息处理函数名为“fix_<消息号>”
    oss.str("");
    oss << "fix_" << msgType;
    std::string luaFun = oss.str();

    lua_getglobal(L.get(), luaFun.c_str());     // 获取函数，压入栈中
    if (lua_isnil(L.get(), -1)) {
        ERROR_LOG("Lua function %s not found for msgType %d\n", luaFun.c_str(), msgType);
        return;
    }

    lua_pushlightuserdata(L.get(), this);  // 压入第一个参数
    lua_pushlightuserdata(L.get(), msg.get());  // 压入第二个参数
    lua_pushnumber(L.get(), tag);                 // 压入第三个参数 
    int iRet= lua_pcall(L.get(), 3, 1, 0);// 调用函数，调用完成以后，会将返回值压入栈中，3表示参数个数，1表示返回结果个数。  
    if (iRet) {                     // 调用出错  
        ERROR_LOG("Call lua failed for msgType %d: %s\n", msgType, lua_tostring(L.get(), -1));
        return;  
    }

    // 检查返回值
    if (!lua_isnumber(L.get(), -1)) {
        ERROR_LOG("Unexpected return type for msgType %d\n", msgType);
        lua_pop(L.get(), 1);  // 清空栈
        return;
    }

    int result = lua_tonumber(L.get(), -1);
    lua_pop(L.get(), 1);  // 清空栈

    // 确保在调用 lua_close 之前清理栈
    lua_settop(L.get(), 0);
}
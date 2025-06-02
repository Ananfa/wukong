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

#ifdef USE_MESSAGE_HOTFIX
#include "message_hotfix_manager.h"
#endif

#include <sstream>

using namespace wukong;

MessageTarget::~MessageTarget() {}

void MessageTarget::handleMessage(int32_t msgType, uint16_t tag, const std::string &rawMsg) {
    google::protobuf::Message *proto;
    bool needCoroutine;
    bool needHotfix;
    MessageHandle handle;
    if (!g_MessageHandleManager.getMessageInfo(msgType, proto, needCoroutine, needHotfix, handle)) {
        ERROR_LOG("MessageTarget::handleMessage -- unknown message type: %d\n", msgType);
        return;
    }

    google::protobuf::Message *msg = nullptr;
    if (proto) {
        msg = proto->New();
        if (!msg->ParseFromString(rawMsg)) {
            // 出错处理
            ERROR_LOG("MessageTarget::handleMessage -- parse fail for message: %d\n", msgType);
            delete msg;
            return;
        } else {
            assert(!rawMsg.empty());
        }
    }

    std::shared_ptr<google::protobuf::Message> targetMsg = std::shared_ptr<google::protobuf::Message>(msg);

    if (need_wait_ || wait_messages_.size() > 0) {
        wait_messages_.push_back({msgType, tag, targetMsg, handle, needHotfix});
        return;
    }

    if (needCoroutine) {
        wait_messages_.push_back({msgType, tag, targetMsg, handle, needHotfix});
        RoutineEnvironment::startCoroutine(handleMessageRoutine, this);
        return;
    }

#ifdef USE_MESSAGE_HOTFIX
    if (needHotfix) {
        callHotfix(msgType, tag, targetMsg);
    } else {
        handle(getPtr(), tag, targetMsg);
    }
#else
    handle(getPtr(), tag, targetMsg);
#endif
}

void *MessageTarget::handleMessageRoutine(void *arg) {
    std::shared_ptr<MessageTarget> self = ((MessageTarget *)arg)->getPtr();

    self->needWait(true);

    do {
        auto &msgInfo = self->wait_messages_.front();

#ifdef USE_MESSAGE_HOTFIX
        if (msgInfo.needHotfix) {
            self->callHotfix(msgInfo.msgType, msgInfo.tag, msgInfo.targetMsg);
        } else {
            msgInfo.handle(self, msgInfo.tag, msgInfo.targetMsg);
        }
#else
        msgInfo.handle(self, msgInfo.tag, msgInfo.targetMsg);
#endif

        self->wait_messages_.pop_front();
    } while (self->wait_messages_.size() > 0);

    self->needWait(false);

    return nullptr;
}

#ifdef USE_MESSAGE_HOTFIX
void MessageTarget::callHotfix(int32_t msgType, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg) {
    LuaStateInfo lsInfo;
    if (!g_MessageHotfixManager.getLuaStateInfo(lsInfo)) {
        ERROR_LOG("getLuaStateInfo failed\n");
        return;
    }

    std::unique_ptr<lua_State, decltype(&lua_close)> L(lsInfo.L, lua_close);
    if (!L) {
        ERROR_LOG("luaL_newstate failed\n");
        return;
    }

    // 获取lua消息处理，约定的消息处理函数名为“fix_<消息号>”
    std::ostringstream oss;
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
    if (lua_pcall(L.get(), 3, 1, 0) != LUA_OK) {  // 调用函数，调用完成以后，会将返回值压入栈中，3表示参数个数，1表示返回结果个数
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

    L.release();
    g_MessageHotfixManager.backLuaStateInfo(lsInfo);
}
#endif
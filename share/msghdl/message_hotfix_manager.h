/*
 * Created by Xianke Liu on 2025/5/22.
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

#ifndef wukong_message_hotfix_manager_h
#define wukong_message_hotfix_manager_h

#include "corpc_redis.h"
#include "message_target.h"
#include "share/define.h"

#include <google/protobuf/message.h>

#include <list>
#include <map>

extern "C"
{  
    #include "lua.h"  
    #include "lauxlib.h"  
    #include "lualib.h"  
}

using namespace corpc;

namespace wukong {

    struct LuaStateInfo {
        lua_State *L;
        time_t lastUsedAt;
        int32_t version;
    };

    class MessageHotfixManager
    {
    public:
        static MessageHotfixManager& Instance() {
            static MessageHotfixManager instance;
            return instance;
        }

        void init();
        
        bool getLuaStateInfo(LuaStateInfo &info);
        void backLuaStateInfo(LuaStateInfo info);

    private:
        void resetHotfix();

        static void *updateRoutine(void * arg);

    private:
        std::map<int32_t, std::string> hotfixMap_;

        std::list<LuaStateInfo> luaStates_;

        int32_t hotfixVersion_;

    private:
        MessageHotfixManager(): hotfixVersion_(0) {}                                   // ctor hidden
        ~MessageHotfixManager() = default;                                             // destruct hidden
        MessageHotfixManager(MessageHotfixManager const&) = delete;                    // copy ctor delete
        MessageHotfixManager(MessageHotfixManager &&) = delete;                        // move ctor delete
        MessageHotfixManager& operator=(MessageHotfixManager const&) = delete;         // assign op. delete
        MessageHotfixManager& operator=(MessageHotfixManager &&) = delete;             // move assign op. delete
    };

}

#define g_MessageHotfixManager wukong::MessageHotfixManager::Instance()

#endif /* wukong_message_hotfix_manager_h */

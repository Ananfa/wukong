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

#ifndef wukong_message_handle_manager_h
#define wukong_message_handle_manager_h

#include "corpc_redis.h"
#include "message_target.h"
#include "share/define.h"
//#include "global_event.h"

//#include <google/protobuf/message.h>

#include <map>
//#include <mutex>
//#include <atomic>
//#include <functional>

//extern "C"  
//{  
//    #include "lua.h"  
//    #include "lauxlib.h"  
//    #include "lualib.h"  
//}

using namespace corpc;

namespace wukong {

    //struct LuaStateInfo {
    //    lua_State *L;
    //    time_t lastUsedAt;
    //    int32_t version;
    //};

    class MessageHandleManager
    {
        struct RegisterMessageInfo {
            google::protobuf::Message *proto;
            MessageHandle handle;
            bool needCoroutine;
            bool needHotfix;
        };

    public:
        static MessageHandleManager& Instance() {
            static MessageHandleManager instance;
            return instance;
        }

        //void init();
        
        bool registerMessage(int32_t msgType,
                             google::protobuf::Message *proto,
                             bool needCoroutine,
                             MessageHandle handle);
        bool isRegistedMessage(int32_t msgType);
        bool getMessageInfo(int32_t msgType, google::protobuf::Message *&proto, bool &needCoroutine, bool &needHotfix, MessageHandle &handle);

        void clearNeedHotfix();
        void setNeedHotfix(int32_t msgType);

        //bool getLuaStateInfo(LuaStateInfo &info);
        //void backLuaStateInfo(LuaStateInfo info);

    //private:
    //    void resetHotfix();
    //
    //    static void *updateRoutine(void * arg);

    private:
        std::map<int32_t, RegisterMessageInfo> registerMessageMap_;
        //std::map<int, std::string> hotfixMap_;
        //
        //std::list<LuaStateInfo> luaStates_;
        //
        //int32_t hotfixVersion_;

    private:
        MessageHandleManager() {}                                                      // ctor hidden
        ~MessageHandleManager() = default;                                             // destruct hidden
        MessageHandleManager(MessageHandleManager const&) = delete;                    // copy ctor delete
        MessageHandleManager(MessageHandleManager &&) = delete;                        // move ctor delete
        MessageHandleManager& operator=(MessageHandleManager const&) = delete;         // assign op. delete
        MessageHandleManager& operator=(MessageHandleManager &&) = delete;             // move assign op. delete
    };
}

#define g_MessageHandleManager wukong::MessageHandleManager::Instance()

#endif /* wukong_message_handle_manager_h */

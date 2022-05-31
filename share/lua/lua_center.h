/*
 * Created by Xianke Liu on 2022/5/19.
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

#ifndef wukong_lua_center_h
#define wukong_lua_center_h

#include "corpc_mutex.h"
#include <string>

extern "C"  
{  
    #include "lua.h"  
    #include "lauxlib.h"  
    #include "lualib.h"  
}

namespace wukong {
    typedef std::function<void (lua_State *L)> LuaBindHandle; // 为lua句柄设置c接口绑定

    class LuaCenter {
    public:
        static LuaCenter& Instance() {
            static LuaCenter instance;
            return instance;
        }

        // init操作接收c绑定方法回调用于Lua句柄产生时设置c接口方法，接收Lua脚本路径用于Lua句柄产生时加载lua程序
        void init(LuaBindHandle luaBind, const std::string &luaPath, GlobalEventListener &listener);

        lua_State* take();
        void put(lua_State *L, bool error);

        bool hasLuaBinding(int msgID);

    private:
        //static void *updateLuaMsgDataRoutine(void * arg);
        static void updateLuaMsgData();
        void refreshLuaMsgData();

        void updateLuaMsgDataVersion() { _LuaMsgDataVersion++; }

    private:
        // lua池
        LuaPool *_pool;

    private:
        // 注意：本服务器框架中引入lua主要是用来解决线上问题热修复，而不是新功能热更上线
        // 当消息需要用lua脚本来处理时，先将脚本部署到服务器中，然后更新lua消息绑定表
        //
        // 当lua脚本改变的时候
        // 1. 将lua脚本更新到lua脚本目录中
        // 2. 发全服事件"LuaChange"
        // 3. 服务器收到全服"LuaChange"事件时，增加lua脚本版本号
        // 4. 当向Lua池获取Lua句柄时，如果版本号不一致，销毁句柄并重新生成
        //
        // 当lua消息绑定表需要更新时
        // 1. 修改Redis中的lua消息绑定表数据，并发全服事件"LuaMsg"
        // 2. 服务器收到全服"LuaMsg"事件时，并从Redis中加载并更新lua消息绑定表
        static std::map<int, bool> _luaMsgIdMap; // lua消息绑定信息（从Redis中获得数据）
        static Mutex _luaMsgDataLock;
        static std::atomic<uint32_t> _luaMsgDataVersion;
        static thread_local uint32_t _t_luaMsgDataVersion;
        static thread_local std::map<int, bool> _t_luaMsgIdMap; // lua脚本绑定的消息号列表（约定处理方法名：mh_<msgID>）

    private:
        LuaCenter() = default;                                   // ctor hidden
        ~LuaCenter() = default;                                  // destruct hidden
        LuaCenter(LuaCenter const&) = delete;                    // copy ctor delete
        LuaCenter(LuaCenter &&) = delete;                        // move ctor delete
        LuaCenter& operator=(LuaCenter const&) = delete;         // assign op. delete
        LuaCenter& operator=(LuaCenter &&) = delete;             // move assign op. delete
    };

    class LuaPool : public thirdparty::ThirdPartyService {
        struct IdleHandle {
            lua_State *handle;
            time_t time;  // 开始idle的时间
        };
        
    public:
        class Proxy {
        public:
            lua_State* take();
            void put(lua_State* lua, bool error);
            
        private:
            Proxy(): _stub(nullptr) {}
            ~Proxy();
            
            void init(InnerRpcServer *server);
            
        private:
            thirdparty::ThirdPartyService::Stub *_stub;
            
        public:
            friend class LuaPool;
        };
        
    public:
        virtual void take(::google::protobuf::RpcController* controller,
                          const Void* request,
                          thirdparty::TakeResponse* response,
                          ::google::protobuf::Closure* done);
        virtual void put(::google::protobuf::RpcController* controller,
                         const thirdparty::PutRequest* request,
                         Void* response,
                         ::google::protobuf::Closure* done);
        
    public:
        static LuaPool* create(LuaBindHandle luaBind, const std::string &luaPath);

        void incSerial() { _luaSerial++; }
        
    private:
        LuaPool(LuaBindHandle luaBind, const std::string &luaPath): _luaBind(luaBind), _luaPath(luaPath) {}
        ~LuaPool() {}
        
        void init();
        
        static void *clearIdleRoutine( void *arg );

        lua_State *newLuaState();
        
    public:
        Proxy proxy;
        
    private:
        LuaBindHandle _luaBind; // c接口绑定delegate
        std::string _luaPath; // lua脚本路径

        uint32_t _luaSerial = 0; // lua脚本变化序列号（启动时从0开始，每次变化加1）
        
        std::list<IdleHandle> _idleList; // 空闲连接表
        std::list<stCoRoutine_t*> _waitingList; // 等待队列：当连接数量达到最大时，新的请求需要等待
        
        InnerRpcServer *_server;
    };
}

#define g_LuaCenter LuaCenter::Instance()

#endif /* wukong_lua_center_h */

// TODO: 监听lua消息绑定表的变化
// TODO: 使用thread_local记录lua消息绑定表数据（参考login服中对server group数据的处理）
// TODO: LuaPool
// TODO: 如何载入c接口？提供一个delegate方法用于初始化lua栈
// TODO: 如何热更lua脚本模块？指定脚本目录，初始化lua栈时或者接到事件时从脚本目录重新加载所有脚本（可以重新创建lua栈）
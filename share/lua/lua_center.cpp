/*
 * Created by Xianke Liu on 2022/5/20.
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

#include "lua_center.h"
#include "corpc_pubsub.h"

using namespace corpc;
using namespace wukong;

std::map<int, bool> LuaCenter::_luaMsgIdMap;
Mutex LuaCenter::_luaMsgDataLock;
std::atomic<uint32_t> LuaCenter::_luaMsgDataVersion(0);
thread_local uint32_t LuaCenter::_t_luaMsgDataVersion(0);
thread_local std::map<int, bool> LuaCenter::_t_luaMsgIdMap;

void LuaCenter::init(LuaBindHandle luaBind, const std::string &luaPath, GlobalEventListener &listener) {
    _pool = LuaPool::create(luaBind, luaPath);

    // 获取lua消息绑定表
    RoutineEnvironment::startCoroutine([](void * arg) -> void* {
        updateLuaMsgData();
        return NULL;
    }, NULL);

    // 订阅“lua脚本改变”主题
    PubsubService::Subscribe("LuaChange", false, [this](const std::string& topic, const std::string& msg) {
        _pool->incSerial();
    });

    // 订阅“lua消息绑定表”主题
    PubsubService::Subscribe("LuaMsg", true, [](const std::string& topic, const std::string& msg) {
        updateLuaMsgData();
    });
}

lua_State* LuaCenter::take() {
    return _pool->proxy.take();
}

void LuaCenter::put(lua_State *L, bool error) {
    _pool->proxy.put(L, error);
}

bool LuaCenter::hasLuaBinding(int msgID) {
    refreshLuaMsgData();
    return _t_luaMsgIdMap.find(msgID) != _t_luaMsgIdMap.end();
}

void LuaCenter::updateLuaMsgData() {
    // 从Redis中加载lua消息绑定表数据更新本地数据
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("LuaCenter::updateLuaMsgData -- connect to cache failed\n");
        return;
    }

    std::string luaMsgData;
    switch (RedisUtils::GetLuaMsgData(cache, luaMsgData)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("LuaCenter::updateLuaMsgData -- get lua msg data failed for db error");
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("LuaCenter::updateLuaMsgData -- get lua msg data failed for invalid data type\n");
            return;
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);
    
    Document doc;
    if (doc.Parse(luaMsgData.c_str()).HasParseError()) {
        ERROR_LOG("LuaCenter::updateLuaMsgData -- parse lua msg data failed\n");
        return;
    }

    if (!doc.IsArray()) {
        ERROR_LOG("LuaCenter::updateLuaMsgData -- parse lua msg data failed for invalid type\n");
        return;
    }

    std::map<int, bool> luaMsgIdMap;
    for (SizeType i = 0; i < doc.Size(); i++) {
        int msgType = doc[i].GetInt();
        if (luaMsgIdMap.find(msgType) == luaMsgIdMap.end()) { // 防重
            luaMsgIdMap.insert(std::make_pair(msgType, true));
        }
    }

    {
        LockGuard lock(_luaMsgDataLock);
        _luaMsgIdMap = std::move(luaMsgIdMap);
        g_LuaCenter.updateLuaMsgDataVersion();
    }

    return;
}

//void *LuaCenter::updateLuaMsgDataRoutine(void * arg) {
//    
//}

void LuaCenter::refreshLuaMsgData() {
    if (_t_luaMsgDataVersion != _luaMsgDataVersion) {
        LOG("refresh lua msg\n");
        {
            LockGuard lock(_luaMsgDataLock);

            if (_t_luaMsgDataVersion == _luaMsgDataVersion) {
                return;
            }

            _t_luaMsgDataVersion = _luaMsgDataVersion;

            _t_luaMsgIdMap.clear();
            for (auto &pair : _luaMsgIdMap) {
                _t_luaMsgIdMap.insert(std::make_pair(pair.first, pair.second));
            }
        }
    }
}














LuaPool::Proxy::~Proxy() {
    if (_stub) {
        delete _stub;
    }
}

void LuaPool::Proxy::init(corpc::InnerRpcServer *server) {
    InnerRpcChannel *channel = new InnerRpcChannel(server);
    
    _stub = new thirdparty::ThirdPartyService::Stub(channel, thirdparty::ThirdPartyService::STUB_OWNS_CHANNEL);
}

lua_State* LuaPool::Proxy::take() {
    Void *request = new Void();
    thirdparty::TakeResponse *response = new thirdparty::TakeResponse();
    Controller *controller = new Controller();
    
    _stub->take(controller, request, response, NULL);
    
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        
        delete controller;
        delete response;
        delete request;
        
        return NULL;
    }
    
    lua_State* lua = (lua_State*)response->handle();
    
    delete controller;
    delete response;
    delete request;
    
    return lua;
}

void LuaPool::Proxy::put(lua_State* lua, bool error) {
    thirdparty::PutRequest *request = new thirdparty::PutRequest();
    Controller *controller = new Controller();
    
    request->set_handle((intptr_t)lua);
    if (error) {
        request->set_error(error);
    }
    
    _stub->put(controller, request, NULL, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

void LuaPool::take(::google::protobuf::RpcController* controller,
                              const Void* request,
                              thirdparty::TakeResponse* response,
                              ::google::protobuf::Closure* done) {
    if (_idleList.size() > 0) {
        intptr_t handle = (intptr_t)_idleList.back().handle;
        _idleList.pop_back();

        // TODO: 若保存的lua版本是旧的，需要重新生成
        lua_State *L = (lua_State *L)handle;
        lua_getglobal(L, "wk_lua_ver");
        int ver = lua_tonumber(L, -1);
        if (ver != _luaSerial) {

        }
        
        response->set_handle(handle);
    } else if (_realConnectCount < _maxConnectNum) {
        // 建立新连接
        _realConnectCount++;
        
        struct timeval timeout = { 3, 0 }; // 3 seconds
        redisContext *redis = redisConnectWithTimeout(_host.c_str(), _port, timeout);
        
        if (redis && !redis->err) {
            // 身份认证
            if (!_passwd.empty()) {
                redisReply *reply = (redisReply *)redisCommand(redis,"AUTH %s", _passwd.c_str());
                if (reply == NULL) {
                    controller->SetFailed("auth failed");

                    redisFree(redis);
                    redis = NULL;
                } else {
                    freeReplyObject(reply);
                }
            }

            if (redis) {
                // 选择分库
                if (_dbIndex) {
                    redisReply *reply = (redisReply *)redisCommand(redis,"SELECT %d", _dbIndex);
                    if (reply == NULL) {
                        controller->SetFailed("can't select index");

                        redisFree(redis);
                        redis = NULL;
                    } else {
                        freeReplyObject(reply);
                        response->set_handle((intptr_t)redis);
                    }
                } else {
                    response->set_handle((intptr_t)redis);
                }
            }
        } else if (redis) {
            std::string reason = "can't connect to redis server for ";
            reason.append(redis->errstr);
            controller->SetFailed(reason);
            
            redisFree(redis);
            redis = NULL;
        } else {
            controller->SetFailed("can't allocate redis context");
        }
        
        if (!redis) {
            // 唤醒所有等待协程
            _realConnectCount--;
            
            while (!_waitingList.empty()) {
                stCoRoutine_t *co = _waitingList.front();
                _waitingList.pop_front();
                
                co_resume(co);
            }
        }
        
    } else {
        // 等待空闲连接
        _waitingList.push_back(co_self());
        co_yield_ct();
        
        if (_idleList.size() == 0) {
            controller->SetFailed("can't connect to redis server");
        } else {
            intptr_t handle = (intptr_t)_idleList.back().handle;
            _idleList.pop_back();

            // TODO: 若保存的lua版本是旧的，需要重新生成
            
            response->set_handle(handle);
        }
    }
}

void LuaPool::put(::google::protobuf::RpcController* controller,
                             const thirdparty::PutRequest* request,
                             Void* response,
                             ::google::protobuf::Closure* done) {
    redisContext *redis = (redisContext *)request->handle();
    
    if (_idleList.size() < _maxConnectNum) {
        if (request->error()) {
            _realConnectCount--;
            redisFree(redis);
            
            // 若有等待协程，尝试重连
            if (_waitingList.size() > 0) {
                assert(_idleList.size() == 0);
                
                if (_realConnectCount < _maxConnectNum) {
                    // 注意: 重连后等待列表可能为空（由其他协程释放连接唤醒列表中的等待协程），此时会出bug，因此先从等待列表中取出一个等待协程
                    stCoRoutine_t *co = _waitingList.front();
                    _waitingList.pop_front();
                    
                    _realConnectCount++;
                    struct timeval timeout = { 1, 500000 }; // 1.5 seconds
                    redisContext *redis = redisConnectWithTimeout(_host.c_str(), _port, timeout);
                    
                    if (redis && !redis->err) {
                        _idleList.push_back({redis, time(nullptr)});
                    } else if (redis) {
                        redisFree(redis);
                        redis = NULL;
                    }
                    
                    if (!redis) {
                        _realConnectCount--;
                    }
                    
                    // 先唤醒先前取出的等待协程
                    co_resume(co);
                    
                    if (!redis) {
                        // 唤醒当前其他所有等待协程
                        while (!_waitingList.empty()) {
                            co = _waitingList.front();
                            _waitingList.pop_front();
                            
                            co_resume(co);
                        }
                    }
                }
            }
        } else {
            _idleList.push_back({redis, time(nullptr)});
            
            if (_waitingList.size() > 0) {
                assert(_idleList.size() == 1);
                stCoRoutine_t *co = _waitingList.front();
                _waitingList.pop_front();
                
                co_resume(co);
            }
        }
    } else {
        assert(_waitingList.size() == 0);
        _realConnectCount--;
        redisFree(redis);
    }
}

LuaPool* LuaPool::create(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum) {
    LuaPool *pool = new LuaPool(host, pwd, port, dbIndex, maxConnectNum);
    pool->init();
    
    return pool;
}

void LuaPool::init() {
    _server = new InnerRpcServer();
    _server->registerService(this);
    _server->start();
    proxy.init(_server);
    
    RoutineEnvironment::startCoroutine(clearIdleRoutine, this);
}

void *LuaPool::clearIdleRoutine( void *arg ) {
    // 定时清理过期连接
    LuaPool *self = (LuaPool*)arg;
    
    time_t now = 0;
    
    while (true) {
        sleep(10);
        
        time(&now);
        
        while (self->_idleList.size() > 0 && self->_idleList.front().time < now - 60) {
            DEBUG_LOG("LuaPool::clearIdleRoutine -- disconnect a redis connection\n");
            redisContext *handle = self->_idleList.front().handle;
            self->_idleList.pop_front();
            self->_realConnectCount--;
            redisFree(handle);
        }
    }
    
    return NULL;
}

lua_State *LuaPool::newLuaState() {
    lua_State *L = luaL_newstate();

    if (L == NULL) {
        return NULL;
    }

    luaL_openlibs(L);

    lua_pushnumber(L, _luaSerial);
    lua_setglobal(L, "wk_lua_ver");

    _luaBind(L);

    // 加载lua脚本目录


}
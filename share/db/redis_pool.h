/*
 * Created by Xianke Liu on 2022/2/24.
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

#ifndef wukong_redis_pool_h
#define wukong_redis_pool_h

#include "corpc_redis.h"

#include <map>
#include <string>

using namespace corpc;

namespace wukong {
    // 注意：RedisPool的实现不考虑销毁流程
    class RedisPool {
        struct InitScriptsContext {
            RedisPool *pool;
            
            std::map<std::string, std::string> scripts;
        };
        
    public:
        static RedisPool* create(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum);

    public:
        RedisConnectPool *getPool() { return _redis; }
        const char *getSha1(const char *sha1Name);

        redisContext *take();
        void put(redisContext* cache, bool error);

        void addScripts(const std::map<std::string, std::string> &scripts);

    private:
        RedisPool() {}
        virtual ~RedisPool() {}

        void init(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum);

        static void *initScriptsRoutine(void *arg);

    private:
        RedisConnectPool *_redis;

        std::map<std::string, std::string> _sha1Map; // 记录所有sha1值
    };

    class RedisPoolManager {
    public:
        static RedisPoolManager& Instance() {
            static RedisPoolManager instance;
            return instance;
        }

        bool addPool(const std::string &poolName, RedisPool* pool); // 注意：此方法不是线程安全的，应该在服务器启动时调用
        RedisPool *getPool(const std::string &poolName);

        bool setCoreCache(const std::string &poolName);
        RedisPool *getCoreCache() { return _coreCache; }
        bool setCorePersist(const std::string &poolName);
        RedisPool *getCorePersist() { return _corePersist; }

    private:
        std::map<std::string, RedisPool*> _poolMap;

        RedisPool *_coreCache = nullptr;
        RedisPool *_corePersist = nullptr;

    private:
        RedisPoolManager() = default;                                       // ctor hidden
        ~RedisPoolManager() = default;                                      // destruct hidden
        RedisPoolManager(RedisPoolManager const&) = delete;                 // copy ctor delete
        RedisPoolManager(RedisPoolManager &&) = delete;                     // move ctor delete
        RedisPoolManager& operator=(RedisPoolManager const&) = delete;      // assign op. delete
        RedisPoolManager& operator=(RedisPoolManager &&) = delete;          // move assign op. delete
    };
}

#define g_RedisPoolManager RedisPoolManager::Instance()

#endif /* wukong_redis_pool_h */

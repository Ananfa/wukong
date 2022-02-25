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

#ifndef redis_pool_h
#define redis_pool_h

#include "corpc_redis.h"

using namespace corpc;

namespace wukong {

    class RedisPool {
    public:
        static RedisPool& Instance() {
            static RedisPool instance;
            return instance;
        }
        
        void init(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum);

        RedisConnectPool *getPool() { return _redis; }

        redisContext *take();
        void put(redisContext* cache, bool error);

        const std::string &bindRoleSha1() { return _bindRoleSha1; }

    private:
        static void *initRoutine(void *arg);

    private:
        RedisConnectPool *_redis;

        std::string _bindRoleSha1; // 添加roleId的lua脚本sha1值

    private:
        RedisPool() = default;                                // ctor hidden
        ~RedisPool() = default;                               // destruct hidden
        RedisPool(RedisPool const&) = delete;                 // copy ctor delete
        RedisPool(RedisPool &&) = delete;                     // move ctor delete
        RedisPool& operator=(RedisPool const&) = delete;      // assign op. delete
        RedisPool& operator=(RedisPool &&) = delete;          // move assign op. delete
    };
}

#define g_RedisPool RedisPool::Instance()

#endif /* redis_pool_h */

/*
 * Created by Xianke Liu on 2021/9/26.
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

#ifndef distribute_lock_h
#define distribute_lock_h

#include "corpc_redis.h"
#include "const.h"

using namespace corpc;

namespace wukong {

    const char UNLOCK_CMD[] = "if redis.call('get', KEYS[1]) == ARGV[1] then return redis.call('del', KEYS[1]) else return 0 end";

    class DistributeLock {
    public:
        DistributeLock(RedisConnectPool *pool, std::string &lockKey);
        ~DistributeLock() {}
        
        bool lock();
        bool unlock();
    private:
        RedisConnectPool *_pool;
        std::string _key;
        uint64_t _identifier;
        bool _locked = false;
    };

    class LockStrategy {
    public:
        LockStrategy() {}
        virtual ~LockStrategy() {}

        virtual bool tryLock(DistributeLock &lock) = 0;
    }

    // 简单分布式锁策略（SimpleLockStrategy）：上锁失败时间隔interval毫秒后重新尝试上锁，最多尝试retryTimes次
    class SimpleLockStrategy: public LockStrategy {
    public:
        SimpleLockStrategy(uint32_t retryTimes, uint32_t interval): LockStrategy(), _retryTimes(retryTimes), _interval(interval) {}
        virtual ~SimpleLockStrategy() {}

        virtual bool tryLock(DistributeLock &lock);

    private:
        uint32_t _retryTimes;
        uint32_t _interval;
    }
}

#endif /* distribute_lock_h */

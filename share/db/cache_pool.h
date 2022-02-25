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

#ifndef cache_pool_h
#define cache_pool_h

#include "corpc_redis.h"

using namespace corpc;

namespace wukong {

    class CachePool {
    public:
        static CachePool& Instance() {
            static CachePool instance;
            return instance;
        }
        
        void init(const char *host, const char *pwd, uint16_t port, uint16_t dbIndex, uint32_t maxConnectNum);

        RedisConnectPool *getPool() { return _cache; }

        redisContext *take();
        void put(redisContext* cache, bool error);

        const std::string &setPassportSha1() { return _setPassportSha1; }
        const std::string &checkPassportSha1() { return _checkPassportSha1; }

        const std::string &loadRoleSha1() { return _loadRoleSha1; }
        const std::string &saveRoleSha1() { return _saveRoleSha1; }
        const std::string &updateRoleSha1() { return _updateRoleSha1; }

        const std::string &setSessionSha1() { return _setSessionSha1; }
        const std::string &setSessionExpireSha1() { return _setSessionExpireSha1; }
        const std::string &removeSessionSha1() { return _removeSessionSha1; }

        const std::string &setRecordSha1() { return _setRecordSha1; }
        const std::string &removeRecordSha1() { return _removeRecordSha1; }
        const std::string &setRecordExpireSha1() { return _setRecordExpireSha1; }

        const std::string &saveProfileSha1() { return _saveProfileSha1; }
        const std::string &updateProfileSha1() { return _updateProfileSha1; }

        const std::string &setLocationSha1() { return _setLocationSha1; }
        const std::string &removeLocationSha1() { return _removeLocationSha1; }
        const std::string &updateLocationSha1() { return _updateLocationSha1; }
        const std::string &setLocationExpireSha1() { return _setLocationExpireSha1; }

    private:
        static void *initRoutine(void *arg);

    private:
        RedisConnectPool *_cache;

        std::string _setPassportSha1; // 设置passport的lua脚本sha1值
        std::string _checkPassportSha1; // 校验passport的lua脚本sha1值
        
        std::string _loadRoleSha1; // 加载role的lua脚本sha1值
        std::string _saveRoleSha1; // 保存profile的lua脚本sha1值
        std::string _updateRoleSha1; // 更新角色数据的lua脚本sha1值

        std::string _setSessionSha1; // 设置session的lua脚本sha1值
        std::string _setSessionExpireSha1; // 设置session超时的lua脚本sha1值
        std::string _removeSessionSha1; // 删除session的lua脚本sha1值

        std::string _setRecordSha1; // 设置Record key的lua脚本sha1值
        std::string _removeRecordSha1; // 删除Record key的lua脚本sha1值
        std::string _setRecordExpireSha1; // 设置Record key超时的lua脚本sha1值

        std::string _saveProfileSha1; // 保存profile的lua脚本sha1值
        std::string _updateProfileSha1; // 更新画像数据的lua脚本sha1值

        std::string _setLocationSha1; // 设置location的lua脚本sha1值
        std::string _removeLocationSha1; // 删除location的lua脚本sha1值
        std::string _updateLocationSha1; // 更新location的lua脚本sha1值
        std::string _setLocationExpireSha1; // 设置location的超时lua脚本sha1值

    private:
        CachePool() = default;                                // ctor hidden
        ~CachePool() = default;                               // destruct hidden
        CachePool(CachePool const&) = delete;                 // copy ctor delete
        CachePool(CachePool &&) = delete;                     // move ctor delete
        CachePool& operator=(CachePool const&) = delete;      // assign op. delete
        CachePool& operator=(CachePool &&) = delete;          // move assign op. delete
    };
}

#define g_CachePool CachePool::Instance()

#endif /* cache_pool_h */

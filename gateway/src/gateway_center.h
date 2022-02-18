/*
 * Created by Xianke Liu on 2020/12/23.
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

#ifndef gateway_center_h
#define gateway_center_h

#include "corpc_redis.h"
#include "share/define.h"

#include <vector>
#include <mutex>
#include <atomic>

using namespace corpc;

namespace wukong {
    class GatewayCenter
    {
    public:
        static GatewayCenter& Instance() {
            static GatewayCenter instance;
            return instance;
        }

        void init();
        
        RedisConnectPool *getCachePool() { return _cache; }

        const std::string &checkPassportSha1() { return _checkPassportSha1; }
        const std::string &setSessionSha1() { return _setSessionSha1; }
        const std::string &setSessionExpireSha1() { return _setSessionExpireSha1; }
        const std::string &removeSessionSha1() { return _removeSessionSha1; }

    private:
        static void *initRoutine(void *arg);

    private:
        RedisConnectPool *_cache;

        std::string _checkPassportSha1; // 校验passport的lua脚本sha1值
        std::string _setSessionSha1; // 设置session的lua脚本sha1值
        std::string _setSessionExpireSha1; // 设置session超时的lua脚本sha1值
        std::string _removeSessionSha1; // 删除session的lua脚本sha1值

    private:
        GatewayCenter() = default;                                       // ctor hidden
        ~GatewayCenter() = default;                                      // destruct hidden
        GatewayCenter(GatewayCenter const&) = delete;                    // copy ctor delete
        GatewayCenter(GatewayCenter &&) = delete;                        // move ctor delete
        GatewayCenter& operator=(GatewayCenter const&) = delete;         // assign op. delete
        GatewayCenter& operator=(GatewayCenter &&) = delete;             // move assign op. delete
    };
}

#define g_GatewayCenter GatewayCenter::Instance()

#endif /* gateway_center_h */

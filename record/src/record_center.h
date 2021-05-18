/*
 * Created by Xianke Liu on 2021/5/6.
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

#ifndef record_center_h
#define record_center_h

#include "corpc_redis.h"
#include "share/define.h"
#include "record_manager.h"

#include <vector>
#include <mutex>
#include <atomic>

using namespace corpc;

namespace wukong {
    typedef std::function<std::shared_ptr<RecordObject> (RoleId, uint32_t, RecordManager*, const std::string &data)> CreateRecordObjectHandler;

    class RecordCenter
    {
    public:
        static RecordCenter& Instance() {
            static RecordCenter instance;
            return instance;
        }

        void init();
        
        RedisConnectPool *getCachePool() { return _cache; }

        const std::string &setRecordSha1() { return _setRecordSha1; }
        const std::string &setRecordExpireSha1() { return _setRecordExpireSha1; }

        void setCreateRecordObjectHandler(CreateRecordObjectHandler handler) { _createRecordObjectHandler = handler; }
        CreateRecordObjectHandler getCreateRecordObjectHandler() { return _createRecordObjectHandler; }

    private:
        static void *initRoutine(void *arg);

    private:
        RedisConnectPool *_cache;

        std::string _setRecordSha1; // 设置Record key的lua脚本sha1值
        std::string _setRecordExpireSha1; // 设置Record key的超时lua脚本sha1值

        CreateRecordObjectHandler _createRecordObjectHandler;
    private:
        RecordCenter() = default;                                      // ctor hidden
        ~RecordCenter() = default;                                     // destruct hidden
        RecordCenter(RecordCenter const&) = delete;                    // copy ctor delete
        RecordCenter(RecordCenter &&) = delete;                        // move ctor delete
        RecordCenter& operator=(RecordCenter const&) = delete;         // assign op. delete
        RecordCenter& operator=(RecordCenter &&) = delete;             // move assign op. delete
    };
}

#define g_RecordCenter RecordCenter::Instance()

#endif /* record_center_h */

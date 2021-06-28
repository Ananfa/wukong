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
#include "corpc_mysql.h"
#include "share/define.h"
#include "record_delegate.h"

#include <vector>
#include <mutex>
#include <atomic>

using namespace corpc;

namespace wukong {
    class RecordCenter
    {
    public:
        static RecordCenter& Instance() {
            static RecordCenter instance;
            return instance;
        }

        void init();
        
        RedisConnectPool *getCachePool() { return _cache; }
        MysqlConnectPool *getMysqlPool() { return _mysql; }

        const std::string &setRecordSha1() { return _setRecordSha1; }
        const std::string &removeRecordSha1() { return _removeRecordSha1; }
        const std::string &setRecordExpireSha1() { return _setRecordExpireSha1; }
        const std::string &updateProfileSha1() { return _updateProfileSha1; }
        const std::string &updateRoleSha1() { return _updateRoleSha1; }
        const std::string &loadRoleSha1() { return _loadRoleSha1; }
        const std::string &saveRoleSha1() { return _saveRoleSha1; }

        void setDelegate(RecordDelegate delegate) { _delegate = delegate; }
        CreateRecordObjectHandle getCreateRecordObjectHandle() { return _delegate.createRecordObject; }
        MakeProfileHandle getMakeProfileHandle() { return _delegate.makeProfile; }

    private:
        static void *initRoutine(void *arg);

        // TODO: 开一个协程定期将cache中数据落地到mysql中

    private:
        RedisConnectPool *_cache;
        MysqlConnectPool *_mysql;

        std::string _setRecordSha1; // 设置Record key的lua脚本sha1值
        std::string _removeRecordSha1; // 删除Record key的lua脚本sha1值
        std::string _setRecordExpireSha1; // 设置Record key超时的lua脚本sha1值
        std::string _updateProfileSha1; // 更新画像数据的lua脚本sha1值
        std::string _updateRoleSha1; // 更新角色数据的lua脚本sha1值
        std::string _loadRoleSha1; // 加载角色数据的lua脚本sha1值
        std::string _saveRoleSha1; // 保存profile的lua脚本sha1值

        RecordDelegate _delegate;
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

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

#ifndef wukong_record_center_h
#define wukong_record_center_h

#include "corpc_semaphore.h"
#include "share/define.h"
#include "share/const.h"
#include "record_delegate.h"

#include <vector>
//#include <mutex>
#include <atomic>

using namespace corpc;

namespace wukong {
    class RecordCenter
    {
        struct WorkerTask {
            RecordCenter *center;
            RoleId roleId;
            uint32_t wheelPos;
        };

    public:
        static RecordCenter& Instance() {
            static RecordCenter instance;
            return instance;
        }

        void init();
        
    private:
        // TODO: 开一个协程定期将cache中数据落地到mysql中
        static void *saveRoutine(void *arg);
        static void *saveWorkerRoutine(void *arg);

    private:
        Semaphore saveSema_;

    private:
        RecordCenter(): saveSema_(MAX_SAVE_WORKER_NUM) {}              // ctor hidden
        ~RecordCenter() = default;                                     // destruct hidden
        RecordCenter(RecordCenter const&) = delete;                    // copy ctor delete
        RecordCenter(RecordCenter &&) = delete;                        // move ctor delete
        RecordCenter& operator=(RecordCenter const&) = delete;         // assign op. delete
        RecordCenter& operator=(RecordCenter &&) = delete;             // move assign op. delete
    };
}

#define g_RecordCenter wukong::RecordCenter::Instance()

#endif /* wukong_record_center_h */

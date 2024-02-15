/*
 * Created by Xianke Liu on 2021/6/10.
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

#ifndef wukong_record_delegate_h
#define wukong_record_delegate_h

#include <functional>
#include "record_object_manager.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    class RecordDelegate {
        typedef std::function<std::shared_ptr<RecordObject> (UserId, RoleId, ServerId, const std::string &rToken, RecordObjectManager*, std::list<std::pair<std::string, std::string>>)> CreateRecordObjectHandle;
        typedef std::function<void (const std::list<std::pair<std::string, std::string>>&, std::list<std::pair<std::string, std::string>>&)> MakeProfileHandle;

    public:
        static RecordDelegate& Instance() {
            static RecordDelegate instance;
            return instance;
        }

        void setCreateRecordObjectHandle(CreateRecordObjectHandle handle) { createRecordObject_ = handle; }
        CreateRecordObjectHandle getCreateRecordObjectHandle() { return createRecordObject_; }
        void setMakeProfileHandle(MakeProfileHandle handle) { makeProfile_ = handle; }
        MakeProfileHandle getMakeProfileHandle() { return makeProfile_; }

    private:
        CreateRecordObjectHandle createRecordObject_;
        MakeProfileHandle makeProfile_;

    private:
        RecordDelegate() = default;                                   // ctor hidden
        RecordDelegate(RecordDelegate const&) = delete;               // copy ctor hidden
        RecordDelegate(RecordDelegate &&) = delete;                   // move ctor hidden
        RecordDelegate& operator=(RecordDelegate const&) = delete;    // assign op. hidden
        RecordDelegate& operator=(RecordDelegate &&) = delete;        // move assign op. hidden
        ~RecordDelegate() = default;                                  // dtor hidden
    };
}

#define g_RecordDelegate wukong::RecordDelegate::Instance()

#endif /* wukong_record_delegate_h */

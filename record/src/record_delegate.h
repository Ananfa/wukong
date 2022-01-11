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

#ifndef record_delegate_h
#define record_delegate_h

#include <functional>
#include "record_object_manager.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {
    typedef std::function<std::shared_ptr<RecordObject> (RoleId, ServerId, uint32_t, RecordObjectManager*, std::list<std::pair<std::string, std::string>>)> CreateRecordObjectHandle;
    typedef std::function<void (const std::list<std::pair<std::string, std::string>>&, std::list<std::pair<std::string, std::string>>&)> MakeProfileHandle;

    struct RecordDelegate {
        CreateRecordObjectHandle createRecordObject;
        MakeProfileHandle makeProfile;
    };

}

#endif /* record_delegate_h */

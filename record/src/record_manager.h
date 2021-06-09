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

#ifndef record_manager_h
#define record_manager_h

#include "corpc_message_server.h"
#include "corpc_redis.h"
#include "timelink.h"
#include "record_object.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {

    class RecordManager {
    public:
        RecordManager(ServerId id):_id(id), _shutdown(false) {}

        void init();

        ServerId getId() { return _id; }

        void shutdown();
        bool isShutdown() { return _shutdown; }

        size_t size(); // 获取当前游戏对象数

        bool exist(RoleId roleId); 

        std::shared_ptr<RecordObject> getRecordObject(RoleId roleId); // 获取玩家存储对象
        std::shared_ptr<RecordObject> create(RoleId roleId, uint32_t rToken, const std::string &data);
        bool remove(RoleId roleId); // 删除玩家游戏对象

    private:
        ServerId _id;       // gateway服务号
        bool _shutdown;

        // 正常网关对象列表
        std::map<RoleId, std::shared_ptr<RecordObject>> _roleId2RecordObjectMap;
    };

}

#endif /* record_manager_h */

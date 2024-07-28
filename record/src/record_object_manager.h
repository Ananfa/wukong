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

#ifndef wukong_record_object_manager_h
#define wukong_record_object_manager_h

#include "corpc_message_server.h"
#include "timelink.h"
#include "record_object.h"
#include "share/define.h"

using namespace corpc;

namespace wukong {

    class RecordObjectManager {
    public:
        static RecordObjectManager& Instance() {
            static RecordObjectManager theSingleton;
            return theSingleton;
        }

    public:
        //RecordObjectManager(ServerId id):id_(id), shutdown_(false) {}

        void init();

        //ServerId getId() { return id_; }

        void shutdown();
        bool isShutdown() { return shutdown_; }

        size_t size(); // 获取当前游戏对象数

        bool exist(RoleId roleId); 

        std::shared_ptr<RecordObject> getRecordObject(RoleId roleId); // 获取玩家存储对象
        std::shared_ptr<RecordObject> create(UserId userId, RoleId roleId, ServerId serverId, const std::string &rToken, std::list<std::pair<std::string, std::string>> &datas);
        bool remove(RoleId roleId); // 删除玩家记录对象

    private:
        //ServerId id_;
        bool shutdown_;

        std::map<RoleId, std::shared_ptr<RecordObject>> roleId2RecordObjectMap_;

    private:
        RecordObjectManager(): shutdown_(false) {}                              // ctor hidden
        RecordObjectManager(RecordObjectManager const&) = delete;               // copy ctor hidden
        RecordObjectManager(RecordObjectManager &&) = delete;                   // move ctor hidden
        RecordObjectManager& operator=(RecordObjectManager const&) = delete;    // assign op. hidden
        RecordObjectManager& operator=(RecordObjectManager &&) = delete;        // move assign op. hidden
        ~RecordObjectManager() = default;                                       // dtor hidden
    };

}

#define g_RecordObjectManager wukong::RecordObjectManager::Instance()

#endif /* wukong_record_object_manager_h */

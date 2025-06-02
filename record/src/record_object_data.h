/*
 * Created by Xianke Liu on 2025/6/1.
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

#ifndef wukong_record_object_data_h
#define wukong_record_object_data_h

#include <list>
#include <string>
#include "record_service.pb.h"

namespace wukong {
    class RecordObjectData {
    public:
        RecordObjectData() {}
        virtual ~RecordObjectData() {}

        virtual bool initData(const std::list<std::pair<std::string, std::string>> &datas) = 0;
        virtual void syncIn(const ::wukong::pb::SyncRequest* request) = 0;
        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas) = 0;
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) = 0;

        void clearDirty() { dirty_map_.clear(); }

    protected:
        std::map<std::string, bool> dirty_map_;
    };
}

#endif /* wukong_lobby_object_data_h */

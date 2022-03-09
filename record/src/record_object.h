/*
 * Created by Xianke Liu on 2021/5/10.
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

#ifndef record_object_h
#define record_object_h

#include "corpc_cond.h"
#include "record_service.pb.h"
#include "share/define.h"
#include <list>
#include <map>

using namespace corpc;

namespace wukong {
    class RecordObject;
    class RecordObjectManager;

    struct RecordObjectRoutineArg {
        std::shared_ptr<RecordObject> obj;
    };

    class RecordObject: public std::enable_shared_from_this<RecordObject> {
    public:
        RecordObject(RoleId roleId, ServerId serverId, const std::string &rToken, RecordObjectManager *manager): _roleId(roleId), _serverId(serverId), _rToken(rToken), _manager(manager), _running(false), _saveTM(0), _cacheFailNum(0) {}
        virtual ~RecordObject() = 0;

        virtual bool initData(const std::list<std::pair<std::string, std::string>> &datas) = 0;

        virtual void syncIn(const ::wukong::pb::SyncRequest* request) = 0;

        RoleId getRoleId() { return _roleId; }
        void setLToken(const std::string &lToken) { _lToken = lToken; }
        const std::string& getLToken() { return _lToken; }
        ServerId getServerId() { return _serverId; }

        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳

        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas) = 0;
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) = 0;

    private:
        static void *heartbeatRoutine(void *arg);  // 心跳协程，周期对record key重设超时时间，如果一段时间没收到游戏对象的心跳时可销毁存储对象

        static void *syncRoutine(void *arg); // 存盘协程（每隔一段时间将脏数据存到Redis中）

        bool cacheData(std::list<std::pair<std::string, std::string>> &datas);
        
        bool cacheProfile(std::list<std::pair<std::string, std::string>> &profileDatas);

    protected:
        RoleId _roleId;
        ServerId _serverId; // 角色所属区服ID
        std::map<std::string, bool> _dirty_map;

    private:
        std::string _lToken; // 游戏对象唯一标识
        std::string _rToken; // 记录对象唯一标识
        bool _running;

        uint64_t _gameObjectHeartbeatExpire; // 游戏对象心跳超时时间
        uint64_t _saveTM; // 落地时间戳
        int _cacheFailNum; // 累计cache失败计数
        
        Cond _cond;

        RecordObjectManager *_manager; // 关联的manager

    public:
        friend class InnerRecordServiceImpl;
    };
}

#endif /* record_object_h */

/*
 * Created by Xianke Liu on 2021/1/18.
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

#ifndef game_object_h
#define game_object_h

#include "corpc_cond.h"
#include "share/define.h"
#include "gateway_service.pb.h"
#include "record_service.pb.h"
#include <list>
#include <map>

using namespace corpc;

namespace wukong {
    class GameObject;
    class GameObjectManager;

    struct GameObjectRoutineArg {
        std::shared_ptr<GameObject> obj;
    };

    class GameObject: public std::enable_shared_from_this<GameObject> {
    public:
        GameObject(UserId userId, RoleId roleId, ServerId serverId, uint32_t lToken, GameObjectManager *manager): _userId(userId), _roleId(roleId), _serverId(serverId), _lToken(lToken), _manager(manager) {}
        virtual ~GameObject() = 0;

        virtual bool initData(const std::string &data) = 0;

        UserId getUserId() { return _userId; }
        RoleId getRoleId() { return _roleId; }
        uint32_t getLToken() { return _lToken; }

        bool setGatewayServerStub(ServerId sid);
        bool setRecordServerStub(ServerId sid);

        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳

        virtual void update(uint64_t nowSec) = 0; // 周期处理逻辑
        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) = 0;
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) = 0;

        virtual void onEnterGame() = 0;

        void send(int32_t type, uint16_t tag, const std::string &rawMsg);
        void send(int32_t type, uint16_t tag, google::protobuf::Message &msg);
        
    private:
        bool reportGameObjectPos(); // 切场景时向gateway上报游戏对象新所在
        bool heartbeatToGateway();
        bool heartbeatToRecord();

        bool sync(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes);

        static void *heartbeatRoutine(void *arg);  // 心跳协程，周期对location重设超时时间，心跳失败时需通知Manager销毁游戏对象

        static void *syncRoutine(void *arg); // 存盘协程（每隔一段时间将脏数据同步给记录服，协程中退出循环时也会执行一次）

        static void *updateRoutine(void *arg); // 逻辑协程（周期逻辑更新）

    private:
        std::shared_ptr<pb::GatewayService_Stub> _gatewayServerStub; // 网关对象所在服务器stub
        ServerId _gatewayId; // 网关对象所在服务器id
        std::shared_ptr<pb::RecordService_Stub> _recordServerStub; // 网关对象所在服务器stub
        ServerId _recordId; // 记录对象所在服务器id
        uint32_t _lToken;

        bool _running = false;
        int _gwHeartbeatFailCount = 0;

        Cond _cond;
        
        GameObjectManager *_manager; // 关联的manager

    protected:
        UserId _userId;
        RoleId _roleId;
        ServerId _serverId;
        
        std::map<std::string, bool> _dirty_map;

    public:
        friend class InnerGameServiceImpl;
    };
}

#endif /* game_object_h */

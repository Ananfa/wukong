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
#if 0
#ifndef wukong_game_object_h
#define wukong_game_object_h

#include "corpc_cond.h"
#include "share/define.h"
#include "gateway_service.pb.h"
#include "record_service.pb.h"
#include "event.h"
#include <list>
#include <map>
#include <sys/time.h>

using namespace corpc;

namespace wukong {
    class GameObject;
    class GameObjectManager;

    struct GameObjectRoutineArg {
        std::shared_ptr<GameObject> obj;
    };

    class GameObject: public std::enable_shared_from_this<GameObject> {
    public:
        GameObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager *manager): userId_(userId), roleId_(roleId), serverId_(serverId), lToken_(lToken), manager_(manager) {}
        virtual ~GameObject() = 0;

        virtual bool initData(const std::string &data) = 0;

        UserId getUserId() { return userId_; }
        RoleId getRoleId() { return roleId_; }
        const std::string &getLToken() { return lToken_; }

        void setSceneId(const std::string &sceneId) { sceneId_ = sceneId; }
        const std::string &getSceneId() { return sceneId_; }

        bool setGatewayServerStub(ServerId sid);
        bool setRecordServerStub(ServerId sid);

        ServerId getGatewayServerId() { return gatewayId_; }
        ServerId getRecordServerId() { return recordId_; }

        GameObjectManager *getManager() { return manager_; }

        virtual void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes) = 0;
        virtual void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) = 0;

        virtual void update(timeval now) = 0; // 周期处理逻辑（注意：不要有产生协程切换的逻辑）
        virtual void onStart() = 0;
        virtual void onDestory() = 0;
        virtual void onEnterGame() = 0; // 客户端登录进入游戏（gateObj与gameObj建立连接）
        virtual void onOffline() = 0; // 客户端断线（gameObj失去gateObj关联）

        bool isOnline() { return gatewayServerStub_ != nullptr; }

        void send(int32_t type, uint16_t tag, const std::string &rawMsg);
        void send(int32_t type, uint16_t tag, google::protobuf::Message &msg);
        
        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳

        void enterGame();
        void leaveGame();

        // 注意：这里不提供注销事件处理的接口方法，在gameobject销毁（stop）的时候一次性注销所有绑定的事件处理
        void regLocalEventHandle(const std::string &name, EventHandle handle);
        void regGlobalEventHandle(const std::string &name, EventHandle handle);
        
        void fireLocalEvent(const Event &event);
        void fireGlobalEvent(const Event &event);

    private:
        //int reportGameObjectPos(); // 切场景时向gateway上报游戏对象新所在
        int heartbeatToGateway();
        int heartbeatToRecord();

        bool sync(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes);

        static void *heartbeatRoutine(void *arg);  // 心跳协程，周期对location重设超时时间，心跳失败时需通知Manager销毁游戏对象

        static void *syncRoutine(void *arg); // 存盘协程（每隔一段时间将脏数据同步给记录服，协程中退出循环时也会执行一次）

        static void *updateRoutine(void *arg); // 逻辑协程（周期逻辑更新）

    private:
        std::shared_ptr<pb::GatewayService_Stub> gatewayServerStub_; // 网关对象所在服务器stub
        ServerId gatewayId_; // 网关对象所在服务器id
        std::shared_ptr<pb::RecordService_Stub> recordServerStub_; // 网关对象所在服务器stub
        ServerId recordId_; // 记录对象所在服务器id
        std::string lToken_;

        std::string sceneId_; // 大厅服时才为""

        bool running_ = false;
        int gwHeartbeatFailCount_ = 0;
        int enterTimes_ = 0; // 重登次数

        Cond cond_;

        EventEmitter emiter_; // 本地事件分派器
        std::vector<uint32_t> globalEventHandleRefs_; // 用于gameobject销毁时注销注册的全局事件处理

    protected:
        UserId userId_;
        RoleId roleId_;
        ServerId serverId_;
        
        std::map<std::string, bool> dirty_map_;

        GameObjectManager *manager_; // 关联的manager

    public:
        friend class GameObjectManager;
    };
}

#endif /* wukong_game_object_h */
#endif
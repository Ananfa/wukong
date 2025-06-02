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

#ifndef wukong_lobby_object_h
#define wukong_lobby_object_h

#include "corpc_cond.h"
#include "share/define.h"
#include "gateway_service.pb.h"
#include "record_service.pb.h"
#include "event.h"
#include "message_target.h"
#include "lobby_object_data.h"
#include "corpc_timer.h"

#include <list>
#include <map>
#include <sys/time.h>

using namespace corpc;

namespace wukong {
    class LobbyObject;
    class LobbyObjectManager;

    class LobbyObject: public MessageTarget {
    public:
        LobbyObject(UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken): userId_(userId), roleId_(roleId), serverId_(serverId), lToken_(lToken) {}
        virtual ~LobbyObject();

        void setObjectData(std::unique_ptr<LobbyObjectData> &&data) { data_ = std::move(data); }
        LobbyObjectData *getObjectData() { return data_.get(); }

        UserId getUserId() { return userId_; }
        RoleId getRoleId() { return roleId_; }
        const std::string &getLToken() { return lToken_; }

        void setSceneId(const std::string &sceneId) { sceneId_ = sceneId; }
        const std::string &getSceneId() { return sceneId_; }

        void setGatewayServerId(ServerId sid);
        void setRecordServerId(ServerId sid);

        ServerId getGatewayServerId() { return gatewayId_; }
        ServerId getRecordServerId() { return recordId_; }

        void buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas, std::list<std::string> &removes);
        void buildAllDatas(std::list<std::pair<std::string, std::string>> &datas);

        bool isOnline() { return gatewayId_ != 0; }

        void send(int32_t type, uint16_t tag, const std::string &rawMsg);
        void send(int32_t type, uint16_t tag, google::protobuf::Message &msg);
        
        void start(); // 开始心跳，启动心跳协程
        void stop(); // 停止心跳

        void enterGame();
        void leaveGame();

        // 玩家lobby_object内部事件处理
        void regEventHandle(const std::string &name, EventHandle handle);
        void fireEvent(const Event &event);

    private:
        //int reportLobbyObjectPos(); // 切场景时向gateway上报游戏对象新所在
        int heartbeatToGateway();
        int heartbeatToRecord();

        bool sync(const std::list<std::pair<std::string, std::string>> &datas, const std::list<std::string> &removes);

        static void *heartbeatRoutine(void *arg);  // 心跳协程，周期对location重设超时时间，心跳失败时需通知Manager销毁游戏对象

        static void *syncRoutine(void *arg); // 存盘协程（每隔一段时间将脏数据同步给记录服，协程中退出循环时也会执行一次）

        static void *updateRoutine(void *arg); // 逻辑协程（周期逻辑更新）

        // 以下方法需要根据项目需求实现
        void onUpdate(timeval now); // 周期处理逻辑（注意：不要有产生协程切换的逻辑）
        void onStart();
        void onDestory();
        void onEnterGame(); // 客户端登录进入游戏（gateObj与gameObj建立连接）
        void onOffline(); // 客户端断线（gameObj失去gateObj关联）

        // 【测试代码】
        void onTestLocalEvent(const Event &e);
        void onTestLocalEvent1(uint32_t testValue);
        //void onTestGlobalEvent(const Event &e);

    private:
        ServerId gatewayId_; // 网关对象所在服务器id
        ServerId recordId_; // 记录对象所在服务器id
        std::string lToken_;

        std::string sceneId_; // 大厅服时才为""

        bool running_ = false;
        int gwHeartbeatFailCount_ = 0;
        int enterTimes_ = 0; // 重登次数

        Cond cond_;

        EventEmitter emiter_; // 本地事件分派器

        std::shared_ptr<Timer> leaveGameTimer_;

    protected:
        UserId userId_;
        RoleId roleId_;
        ServerId serverId_;

        std::unique_ptr<LobbyObjectData> data_;
        
    public:
        friend class LobbyObjectManager;
    };
}

#endif /* wukong_lobby_object_h */

/*
 * Created by Xianke Liu on 2022/2/16.
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
#ifndef wukong_scene_client_h
#define wukong_scene_client_h

#include <map>
#include <vector>
#include "corpc_rpc_client.h"
#include "corpc_mutex.h"
#include "game_service.pb.h"
#include "scene_service.pb.h"
#include "game_client.h"
#include "define.h"
#include "const.h"

using namespace corpc;

namespace wukong {
    class SceneClient: public GameClient {
    public:
        struct StubInfo {
            std::string rpcAddr; // rpc服务地址"ip:port"
            uint32_t type; // 场景服类型
            std::shared_ptr<pb::GameService_Stub> gameServiceStub;
            std::shared_ptr<pb::SceneService_Stub> sceneServiceStub;
        };

    public:
        static SceneClient& Instance() {
            static SceneClient instance;
            return instance;
        }

        virtual std::vector<ServerInfo> getServerInfos();
        virtual bool setServers(const std::vector<AddressInfo> &addresses);
        virtual void forwardIn(ServerId sid, int16_t type, uint16_t tag, RoleId roleId, const std::string &rawMsg);

        virtual std::shared_ptr<pb::GameService_Stub> getGameServiceStub(ServerId sid);
        
        std::shared_ptr<pb::SceneService_Stub> getSceneServiceStub(ServerId sid);
        
        std::string loadScene(ServerId sid, uint32_t defId, const std::string &sceneId, RoleId roleId, const std::string &teamId);
        void enterScene(ServerId sid, const std::string &sceneId, RoleId roleId, ServerId gwId);
        
        void shutdown();

        bool stubChanged() { return stubChangeNum_ != t_stubChangeNum_; }
        
    private:
        void refreshStubs();

    private:
        /* 所有SceneServer的Stub */
        static std::map<std::string, std::pair<std::shared_ptr<pb::GameService_Stub>, std::shared_ptr<pb::SceneService_Stub>>> addr2stubs_; // 用于保持被_stubs中的StubInfo引用（不直接访问）
        static std::map<ServerId, StubInfo> stubs_;
        static Mutex stubsLock_;
        static std::atomic<uint32_t> stubChangeNum_;
        
        /* 当前可用的 */
        static thread_local std::map<ServerId, StubInfo> t_stubs_;
        static thread_local uint32_t t_stubChangeNum_;

    private:
        SceneClient(): GameClient(GAME_SERVER_TYPE_SCENE, ZK_SCENE_SERVER) {} // ctor hidden
        virtual ~SceneClient() = default;                       // destruct hidden
        SceneClient(SceneClient const&) = delete;               // copy ctor delete
        SceneClient(SceneClient &&) = delete;                   // move ctor delete
        SceneClient& operator=(SceneClient const&) = delete;    // assign op. delete
        SceneClient& operator=(SceneClient &&) = delete;        // move assign op. delete
    };
}
    
#define g_SceneClient wukong::SceneClient::Instance()

#endif /* wukong_scene_client_h */
#endif
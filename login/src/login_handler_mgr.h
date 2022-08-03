/*
 * Created by Xianke Liu on 2020/11/20.
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

#ifndef wukong_login_handler_mgr_h
#define wukong_login_handler_mgr_h

#include "login_delegate.h"
#include "http_server.h"
#include "gateway_client.h"
#include "corpc_mutex.h"

#include <map>
#include <atomic>

using namespace corpc;

namespace wukong {
    struct LogicServerInfo {
        uint16_t id; // 逻辑服id
        uint16_t group; // 组号（合服相关，多个逻辑服合服后它们有相同的group，逻辑服入口保留）
        std::string name; // 逻辑服名
    };

    struct RoleServerInfo {
        ServerId serverId;
        RoleId roleId;
    };

    struct RoleProfile {
        ServerId serverId; // 所在服id
        RoleId roleId; // 角色id
        std::string pData; // 角色画像数据
    };

    class LoginHandlerMgr {
    public:
        static LoginHandlerMgr& Instance() {
            static LoginHandlerMgr instance;
            return instance;
        }
        
        void init(HttpServer *server);
        
        void setDelegate(LoginDelegate delegate) { _delegate = delegate; }

        bool randomGatewayServer(ServerId &serverId);
    private:
        // login接口：校验玩家身份，生成临时身份token，获取玩家在各逻辑服中的角色基本信息（角色列表），获取服务器列表，并将以上信息返回给客户端
        void login(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response);
        // getProfile接口：获取角色侧面信息
        void getProfile(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response);
        // createRole接口：创角处理
        void createRole(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response);
        // enterGame接口：进入游戏处理
        void enterGame(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response);

    private:
        void setResponse(std::shared_ptr<ResponseMessage> &response, const std::string &content);
        void setErrorResponse(std::shared_ptr<ResponseMessage> &response, const std::string &content);
        
        void updateServerGroupDataVersion() { _serverGroupDataVersion++; }
        void updateServerGroupData();
        void refreshServerGroupData();

        bool checkToken(UserId userId, const std::string& token);

        static void *saveUserRoutine(void *arg); // 将account-userid对应关系信息存盘的协程

    private:
        static std::string _serverGroupData;
        static Mutex _serverGroupDataLock;
        static std::atomic<uint32_t> _serverGroupDataVersion;

        static thread_local std::string _t_serverGroupData;
        static thread_local uint32_t _t_serverGroupDataVersion;
        static thread_local std::map<GroupId, uint32_t> _t_groupStatusMap;
        static thread_local std::map<ServerId, GroupId> _t_serverId2groupIdMap;

        LoginDelegate _delegate;

    private:
        LoginHandlerMgr() = default;                                     // ctor hidden
        ~LoginHandlerMgr() = default;                                    // destruct hidden
        LoginHandlerMgr(LoginHandlerMgr const&) = delete;                 // copy ctor delete
        LoginHandlerMgr(LoginHandlerMgr &&) = delete;                     // move ctor delete
        LoginHandlerMgr& operator=(LoginHandlerMgr const&) = delete;      // assign op. delete
        LoginHandlerMgr& operator=(LoginHandlerMgr &&) = delete;          // move assign op. delete
    };
}

#define g_LoginHandlerMgr wukong::LoginHandlerMgr::Instance()

#endif /* wukong_login_handler_mgr_h */

/*
 * Created by Xianke Liu on 2021/1/15.
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

#ifndef record_client_h
#define record_client_h

#include <map>
#include <vector>
#include "corpc_rpc_client.h"
#include "record_service.pb.h"
#include "define.h"

using namespace corpc;

namespace wukong {
    class RecordClient {
    public:
        struct AddressInfo {
            uint16_t id;
            std::string ip;
            uint16_t port;
        };

        struct StubInfo {
            std::string ip; // rpc服务ip
            uint16_t port; // rpc服务port
            std::shared_ptr<pb::RecordService_Stub> stub;
        };

        struct ServerInfo {
            ServerId id;
            uint32_t count; // 在线人数
            uint32_t weight; // 分配权重(所有服务器的总在线人数 - 在线人数)
        };

    public:
        static RecordClient& Instance() {
            static RecordClient instance;
            return instance;
        }

        void init(RpcClient *client) { _client = client; }

        /* 业务逻辑 */
        std::vector<ServerInfo> getServerInfos(); // 注意：这里直接定义返回vector类型，通过编译器RVO优化
        bool loadRole(ServerId sid, RoleId roleId, uint32_t lToken, std::string &roleData); // 加载角色（游戏对象）

        /* 加入Server */
        bool setServers(const std::map<ServerId, AddressInfo> &addresses);
        /* 根据逻辑区服id获得RecordServer的stub */
        std::shared_ptr<pb::RecordService_Stub> getStub(ServerId sid);
        
        static bool parseAddress(const std::string &input, AddressInfo &addressInfo);

    private:
        void refreshStubs();

    private:
        RpcClient *_client = nullptr;

        /* 所有LobbyServer的Stub */
        static std::map<ServerId, StubInfo> _stubs;
        static std::mutex _stubsLock;
        static std::atomic<uint32_t> _stubChangeNum;
        
        /* 当前可用的 */
        static thread_local std::map<ServerId, StubInfo> _t_stubs;
        static thread_local uint32_t _t_stubChangeNum;

    private:
        RecordClient() = default;                                // ctor hidden
        ~RecordClient() = default;                               // destruct hidden
        RecordClient(RecordClient const&) = delete;               // copy ctor delete
        RecordClient(RecordClient &&) = delete;                   // move ctor delete
        RecordClient& operator=(RecordClient const&) = delete;    // assign op. delete
        RecordClient& operator=(RecordClient &&) = delete;        // move assign op. delete
    };
}
    
#define g_RecordClient wukong::RecordClient::Instance()

#endif /* record_client_h */

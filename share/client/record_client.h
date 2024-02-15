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

#ifndef wukong_record_client_h
#define wukong_record_client_h

#include <map>
#include <vector>
#include "corpc_rpc_client.h"
#include "corpc_mutex.h"
#include "record_service.pb.h"
#include "define.h"

using namespace corpc;

namespace wukong {
    class RecordClient {
    public:
        struct AddressInfo {
            std::string ip;
            uint16_t port;
            std::vector<uint16_t> serverIds;
        };

        struct StubInfo {
            std::string rpcAddr; // rpc服务地址"ip:port"
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

        void init(RpcClient *client) { client_ = client; }

        /* 业务逻辑 */
        void shutdown();
        std::vector<ServerInfo> getServerInfos(); // 注意：这里直接定义返回vector类型，通过编译器RVO优化
        bool loadRoleData(ServerId sid, RoleId roleId, UserId &userId, const std::string &lToken, ServerId &serverId, std::string &roleData); // 加载角色（游戏对象）

        bool stubChanged() { return stubChangeNum_ != t_stubChangeNum_; }
        /* 加入Server */
        bool setServers(const std::vector<AddressInfo> &addresses);
        /* 根据逻辑区服id获得RecordServer的stub */
        std::shared_ptr<pb::RecordService_Stub> getStub(ServerId sid);
        
        static bool parseAddress(const std::string &input, AddressInfo &addressInfo);

    private:
        void refreshStubs();

    private:
        RpcClient *client_ = nullptr;

        /* 所有RecordServer的Stub */
        static std::map<std::string, std::shared_ptr<pb::RecordService_Stub>> addr2stubs_; // 用于保持被_stubs中的StubInfo引用（不直接访问）
        static std::map<ServerId, StubInfo> stubs_;
        static Mutex stubsLock_;
        static std::atomic<uint32_t> stubChangeNum_;
        
        /* 当前可用的 */
        static thread_local std::map<ServerId, StubInfo> t_stubs_;
        static thread_local uint32_t t_stubChangeNum_;

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

#endif /* wukong_record_client_h */

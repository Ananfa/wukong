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

#ifndef wukong_gateway_client_h
#define wukong_gateway_client_h

#include <map>
#include <vector>
#include "corpc_rpc_client.h"
#include "corpc_mutex.h"
#include "gateway_service.pb.h"
#include "define.h"

using namespace corpc;

namespace wukong {
    class GatewayClient {
    public:
        struct AddressInfo {
            std::string ip;
            uint16_t port;
            std::string outerAddr;
            std::vector<std::pair<uint16_t, uint16_t>> serverPorts;
        };

        struct StubInfo {
            std::string rpcAddr; // rpc服务地址"ip:port"
            std::string outerAddr; // 客户端连接地址的ip或域名
            uint16_t outerPort; // 客户端连接地址的端口
            std::shared_ptr<pb::GatewayService_Stub> stub;
        };

        struct ServerInfo {
            ServerId id;
            std::string outerAddr; // 客户端连接地址的ip或域名
            uint16_t outerPort; // 客户端连接地址的端口
            uint32_t count; // 在线人数
            uint32_t weight; // 分配权重(所有服务器的总在线人数 - 在线人数)
        };
    public:
        static GatewayClient& Instance() {
            static GatewayClient instance;
            return instance;
        }

        void init(RpcClient *client) { client_ = client; }

        /* 业务逻辑 */
        void shutdown();
        bool kick(ServerId sid, UserId userId, const std::string &gToken);
        std::vector<ServerInfo> getServerInfos(); // 注意：这里直接定义返回vector类型，通过编译器RVO优化
        void broadcast(ServerId sid, int32_t type, uint16_t tag, const std::vector<std::pair<UserId, std::string>> &targets, const std::string &rawMsg);
        // TODO: 对全服的broadcast
    
        bool stubChanged() { return stubChangeNum_ != t_stubChangeNum_; }
        /* 加入Server */
        bool setServers(const std::vector<AddressInfo> &addresses);
        /* 根据逻辑区服id获得GameServer的stub */
        std::shared_ptr<pb::GatewayService_Stub> getStub(ServerId sid);
        
        static bool parseAddress(const std::string &input, AddressInfo &addressInfo);

    private:
        void refreshStubs();

    private:
        RpcClient *client_ = nullptr;

        /* 所有GatewayServer的Stub */
        static std::map<std::string, std::shared_ptr<pb::GatewayService_Stub>> addr2stubs_; // 用于保持被_stubs中的StubInfo引用（不直接访问）
        static std::map<ServerId, StubInfo> stubs_;
        static Mutex stubsLock_;
        static std::atomic<uint32_t> stubChangeNum_;
        
        /* 当前可用的 */
        static thread_local std::map<ServerId, StubInfo> t_stubs_;
        static thread_local uint32_t t_stubChangeNum_;

    private:
        GatewayClient() = default;                                  // ctor hidden
        ~GatewayClient() = default;                                 // destruct hidden
        GatewayClient(GatewayClient const&) = delete;               // copy ctor delete
        GatewayClient(GatewayClient &&) = delete;                   // move ctor delete
        GatewayClient& operator=(GatewayClient const&) = delete;    // assign op. delete
        GatewayClient& operator=(GatewayClient &&) = delete;        // move assign op. delete
    };
}
    
#define g_GatewayClient wukong::GatewayClient::Instance()

#endif /* wukong_gateway_client_h */

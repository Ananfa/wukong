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

#ifndef gateway_client_h
#define gateway_client_h

#include <map>
#include <vector>
#include "corpc_rpc_client.h"
#include "gateway_service.pb.h"
#include "define.h"

using namespace corpc;

namespace wukong {
    class GatewayClient {
    public:
        struct AddressInfo {
            uint16_t id;
            std::string ip;
            uint16_t port;
            std::string outerAddr;
            uint16_t outerPort;
        };

        struct StubInfo {
            std::string ip; // rpc服务ip
            uint16_t port; // rpc服务port
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

        void init(RpcClient *client) { _client = client; }

        /* 业务逻辑 */
        void shutdown();
        bool kick(ServerId sid, UserId userId);
        std::vector<ServerInfo> getServerInfos(); // 注意：这里直接定义返回vector类型，通过编译器RVO优化
        void broadcast(ServerId sid, int32_t type, uint16_t tag, const std::vector<std::pair<UserId, uint32_t>> &targets, const std::string &rawMsg);
        // TODO: 对全服的broadcast
    
        /* 加入Server */
        bool setServers(const std::map<ServerId, AddressInfo> &addresses);
        /* 根据逻辑区服id获得GameServer的stub */
        std::shared_ptr<pb::GatewayService_Stub> getStub(ServerId sid);
        
        static bool parseAddress(const std::string &input, AddressInfo &addressInfo);

    private:
        void refreshStubs();

    private:
        RpcClient *_client = nullptr;

        /* 所有GameServer的Stub */
        static std::map<ServerId, StubInfo> _stubs;
        static std::mutex _stubsLock;
        static std::atomic<uint32_t> _stubChangeNum;
        
        /* 当前可用的 */
        static thread_local std::map<ServerId, StubInfo> _t_stubs;
        static thread_local uint32_t _t_stubChangeNum;

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

#endif /* gateway_client_h */

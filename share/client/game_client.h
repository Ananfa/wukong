/*
 * Created by Xianke Liu on 2021/6/8.
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

#ifndef game_client_h
#define game_client_h

#include <map>
#include <vector>
#include "corpc_rpc_client.h"
#include "game_service.pb.h"
#include "define.h"

using namespace corpc;

namespace wukong {
    class GameClient {
    public:
        struct AddressInfo {
            uint16_t id;
            std::string ip;
            uint16_t port;
        };

        struct ServerInfo {
            ServerId id;
            uint32_t count; // 在线人数
            uint32_t weight; // 分配权重(所有服务器的总在线人数 - 在线人数)
        };

    public:
        GameClient(GameServerType gsType, const std::string &zkNodeName): _gsType(gsType), _zkNodeName(zkNodeName) {}
        virtual ~GameClient() = default;

        void init(RpcClient *client) { _client = client; }
        GameServerType getGameServerType() const { return _gsType; }
        const std::string &getZkNodeName() const { return _zkNodeName; }

        virtual std::vector<ServerInfo> getServerInfos() = 0; // 注意：这里直接定义返回vector类型，通过编译器RVO优化
        virtual bool setServers(const std::map<ServerId, AddressInfo> &addresses) = 0;
        virtual void forwardIn(ServerId sid, int16_t type, uint16_t tag, const std::vector<RoleId> &roleIds, const std::string &rawMsg) = 0;
        virtual std::shared_ptr<pb::GameService_Stub> getGameServiceStub(ServerId sid) = 0;

        static bool parseAddress(const std::string &input, AddressInfo &addressInfo);

    protected:
        RpcClient *_client = nullptr;
        GameServerType _gsType;
        std::string _zkNodeName;
    };
}
    
#endif /* game_client_h */

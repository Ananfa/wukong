/*
 * Created by Xianke Liu on 2024/7/11.
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

#ifndef wukong_agent_manager_h
#define wukong_agent_manager_h

#include "corpc_message_client.h"
#include "corpc_routine_env.h"
#include "inner_common.pb.h"
#include "agent.h"

#include <map>

// 负责连接Nexus服务，上报登记本服信息，接收关注的服务器信息，替换client_center功能
// 注意：g_AgentManager对象非线程安全，start后会在IO的worker所在的线程访问，因此IO中的worker不能是多线程的

using namespace corpc;

namespace wukong {
    class AgentManager {
    public:
        static AgentManager& Instance() {
            static AgentManager instance;
            return instance;
        }

        bool init(IO *io, const std::string& nexusHost, uint16_t nexusPort, const pb::ServerInfo &serverInfo);
        bool start();

        void setLocalServerInfo(const pb::ServerInfo& serverInfo) {
            localServerInfoChanged_ = true;
            localServerInfo_ = serverInfo; 
        }

        pb::ServerInfo& getLocalServerInfo() { return localServerInfo_; }
        std::map<ServerId, pb::ServerInfo>& getRemoteServerInfos(ServerType stype);

        void registerAgent(Agent *agent);
        Agent *getAgent(ServerType stype);

    private:
        static void connectHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);
        static void closeHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);
        static void accessRspHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);
        static void svrInfoHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);
        static void rmSvrHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn);

        static void *updateRoutine(void *arg);

    private:
        corpc::MessageTerminal *terminal_;
        corpc::TcpClient *nexusClient_;

        bool inited_;
        bool started_;

        bool localServerInfoChanged_;
        pb::ServerInfo localServerInfo_; // 本地服务信息
        
        std::map<ServerType, std::map<ServerId, pb::ServerInfo>> remoteServerInfos_; // 远程服务器信息

        // 问题：不同服务器xxx_client有不同实现，但在不同服务器中不是每个client都需要被使用，此时包含这些client实现会是代码体积变大
        //       最好是提供统一接口给nexus_client调用，通过注册方式而不是直接访问xxx_client的单例
        std::map<ServerType, Agent*> agents_;

        std::map<ServerId, pb::ServerInfo> emptyServerInfoMap_; // 用于找不到时返回

    private:
        AgentManager(): inited_(false), started_(false), localServerInfoChanged_(false) {}     // ctor hidden
        ~AgentManager() = default;                                    // destruct hidden
        AgentManager(AgentManager const&) = delete;                    // copy ctor delete
        AgentManager(AgentManager &&) = delete;                        // move ctor delete
        AgentManager& operator=(AgentManager const&) = delete;         // assign op. delete
        AgentManager& operator=(AgentManager &&) = delete;             // move assign op. delete
    };
}

#define g_AgentManager wukong::AgentManager::Instance()

#endif /* wukong_agent_manager_h */

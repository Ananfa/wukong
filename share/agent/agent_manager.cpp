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

#include "agent_manager.h"
#include "const.h"

using namespace corpc;
using namespace wukong;

bool AgentManager::init(IO *io, const std::string& nexusHost, uint16_t nexusPort, const pb::ServerInfo &serverInfo) {
    if (inited_) {
        ERROR_LOG("AgentManager::init -- already inited\n");
        return false;
    }

    localServerInfo_ = serverInfo;

    terminal_ = new corpc::MessageTerminal(true, true, true, true);
    nexusClient_ = new corpc::TcpClient(io, nullptr, terminal_, nexusHost, nexusPort);

    terminal_->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false, AgentManager::connectHandle);
    terminal_->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, true, AgentManager::closeHandle);
    terminal_->registerMessage(N2S_MESSAGE_ID_ACCESS_RSP, nullptr, true, AgentManager::accessRspHandle);
    terminal_->registerMessage(N2S_MESSAGE_ID_SVRINFO, nullptr, true, AgentManager::svrInfoHandle);
    terminal_->registerMessage(N2S_MESSAGE_ID_RMSVR, nullptr, true, AgentManager::rmSvrHandle);

    inited_ = true;
    return true;
}

bool AgentManager::start() {
    if (started_) {
        return false;
    }
    started_ = true;
    localServerInfoChanged_ = false;

    nexusClient_->connect();
    RoutineEnvironment::startCoroutine(updateRoutine, this);
}

std::map<ServerId, pb::ServerInfo>& AgentManager::getRemoteServerInfos(ServerType stype) {
    auto it = remoteServerInfos_.find(stype);

    if (it != remoteServerInfos_.end()) {
        return it->second;
    }

    return emptyServerInfoMap_;
}

void AgentManager::registerAgent(Agent *agent) {
    if (agents_.find(agent->getType()) != agents_.end()) {
        ERROR_LOG("duplicate agent: %d\n", agent->getType());
        return;
    }

    agents_.insert(std::make_pair(agent->getType(), agent));
}

Agent *AgentManager::getAgent(ServerType stype) {
    auto it = agents_.find(stype);
    if (it != agents_.end()) {
        return it->second;
    }

    return nullptr;
}

void AgentManager::connectHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    LOG("nexus-server connected\n");
    std::shared_ptr<pb::ServerAccessRequest> accessMsg(new pb::ServerAccessRequest);

    pb::ServerInfo &info = g_AgentManager.getLocalServerInfo();
    pb::ServerInfo* info1 = accessMsg->mutable_server_info();
    *info1 = info;

    conn->send(S2N_MESSAGE_ID_ACCESS, false, false, false, 0, accessMsg);
    
    g_AgentManager.localServerInfoChanged_ = false;
}

void AgentManager::closeHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    LOG("nexus-server disconnected\n");
    while (!g_AgentManager.nexusClient_->connect()) {
        LOG("try reconnect nexus-server\n");
        sleep(1);
    }
}

void AgentManager::accessRspHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    // 初始化本地stub数据
    pb::ServerAccessResponse *response = static_cast<pb::ServerAccessResponse*>(msg.get());

    // 重置服务器信息
    std::map<ServerType, std::map<ServerId, pb::ServerInfo>> newRemoteServerInfos;
    for (auto it = response->server_infos().begin(); it != response->server_infos().end(); ++it) {
        ServerType stype = it->server_type();
        ServerId sid = it->server_id();

        Agent *agent = g_AgentManager.getAgent(stype);
        agent->addStub(it->server_id(), it->rpc_host(), it->rpc_port());

        newRemoteServerInfos[stype].insert(std::make_pair(sid, *it));
    }

    std::list<std::pair<ServerType, ServerId>> removeServers;
    for (auto pair1 : g_AgentManager.remoteServerInfos_) {
        ServerType stype = pair1.first;
        for (auto pair2 : pair1.second) {
            ServerId sid = pair2.first;
            assert(stype == pair2.second.server_type());
            assert(sid == pair2.second.server_id());

            auto it1 = newRemoteServerInfos.find(stype);
            if (it1 != newRemoteServerInfos.end()) {
                auto it2 = it1->second.find(sid);
                if (it2 != it1->second.end()) {
                    continue;
                }
            }

            removeServers.push_back(std::make_pair(stype, sid));
        }
    }

    for (auto pair : removeServers) {
        Agent *agent = g_AgentManager.getAgent(pair.first);
        agent->removeStub(pair.second);
    }

    g_AgentManager.remoteServerInfos_ = std::move(newRemoteServerInfos);
}

void AgentManager::svrInfoHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    pb::ServerInfoNtf *ntf = static_cast<pb::ServerInfoNtf*>(msg.get());

    ServerType stype = ntf->server_info().server_type();
    Agent *agent = g_AgentManager.getAgent(stype);
    agent->addStub(ntf->server_info().server_id(), ntf->server_info().rpc_host(), ntf->server_info().rpc_port());
}

void AgentManager::rmSvrHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    pb::RemoveServerNtf *ntf = static_cast<pb::RemoveServerNtf*>(msg.get());
    Agent *agent = g_AgentManager.getAgent(ntf->server_type());
    agent->removeStub(ntf->server_id());
}

void *AgentManager::updateRoutine(void *arg) {
    // 定时(每10秒)向nexus服上报自身状态（有变化才上报）
    AgentManager *self = (AgentManager*)arg;
    while (true) {
        if (self->localServerInfoChanged_) {
            std::shared_ptr<pb::ServerInfoNtf> ntf(new pb::ServerInfoNtf);

            pb::ServerInfo &info = g_AgentManager.getLocalServerInfo();
            pb::ServerInfo* info1 = ntf->mutable_server_info();
            *info1 = info;

            self->nexusClient_->send(S2N_MESSAGE_ID_UPDATE, false, false, false, 0, ntf);

            self->localServerInfoChanged_ = false;
        }

        sleep(10);
    }
}
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
        ERROR_LOG("agent manager already inited\n");
        return false;
    }

    localServerInfo_ = serverInfo;

    terminal_ = new corpc::MessageTerminal(true, true, true, true);
    nexusClient_ = new corpc::TcpClient(io, nullptr, terminal_, nexusHost, nexusPort);

    terminal_->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false, std::bind(&AgentManager::connectHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    terminal_->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, true, std::bind(&AgentManager::closeHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    terminal_->registerMessage(N2S_MESSAGE_ID_ACCESS_RSP, nullptr, true, std::bind(&AgentManager::accessRspHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    terminal_->registerMessage(N2S_MESSAGE_ID_SVRINFO, nullptr, true, std::bind(&AgentManager::svrInfoHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));
    terminal_->registerMessage(N2S_MESSAGE_ID_RMSVR, nullptr, true, std::bind(&AgentManager::rmSvrHandle, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4));

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
    return true;
}

//void AgentManager::setRemoteServerInfo(const pb::ServerInfo& serverInfo) {
//    remoteServerInfos_[serverInfo.server_type()][serverInfo.server_id()] = serverInfo;
//}
//
//void AgentManager::removeRemoteServerInfo(ServerType stype, ServerId sid) {
//    auto it1 = remoteServerInfos_.find(stype);
//
//    if (it1 == remoteServerInfos_.end()) {
//        WARN_LOG("server[%d:%d] not exist\n", stype, sid);
//        return;
//    }
//
//    it1->second.erase(sid);
//    if (it1->second.empty()) {
//        remoteServerInfos_.erase(it1);
//    }
//}
//
//pb::ServerInfo* AgentManager::getRemoteServerInfo(ServerType stype, ServerId sid) {
//    auto it1 = remoteServerInfos_.find(stype);
//
//    if (it1 == remoteServerInfos_.end()) {
//        return nullptr;
//    }
//
//    auto it2 = it1->second.find(sid);
//    if (it2 == it1->second.end()) {
//        return nullptr;
//    }
//
//    return &it2->second;
//}

void AgentManager::registerAgent(Agent *agent) {
    if (agents_.find(agent->getType()) != agents_.end()) {
        ERROR_LOG("duplicate agent: %d\n", agent->getType());
        return;
    }

    agents_.insert(std::make_pair(agent->getType(), agent));
}

Agent* AgentManager::getAgent(ServerType stype) {
    auto it = agents_.find(stype);
    if (it != agents_.end()) {
        return it->second;
    }

    return nullptr;
}

void AgentManager::connectHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    LOG("nexus-server connected\n");
    std::shared_ptr<pb::ServerAccessRequest> accessMsg(new pb::ServerAccessRequest);

    pb::ServerInfo* info1 = accessMsg->mutable_server_info();
    *info1 = localServerInfo_;

    conn->send(S2N_MESSAGE_ID_ACCESS, false, false, false, 0, accessMsg);
    
    localServerInfoChanged_ = false;
}

void AgentManager::closeHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    LOG("nexus-server disconnected\n");
    while (!nexusClient_->connect()) {
        LOG("try reconnect nexus-server\n");
        sleep(1);
    }
}

void AgentManager::accessRspHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    // 初始化本地stub数据
    pb::ServerAccessResponse *response = static_cast<pb::ServerAccessResponse*>(msg.get());

    // 重置服务器信息
    std::map<ServerType, std::list<pb::ServerInfo>> serverInfos;
    for (auto it = response->server_infos().begin(); it != response->server_infos().end(); ++it) {
        serverInfos[it->server_type()].push_back(*it);
    }

    for (auto &pair : agents_) {
        pair.second->resetStubs(serverInfos[pair.first]);
    }
}

void AgentManager::svrInfoHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    pb::ServerInfoNtf *ntf = static_cast<pb::ServerInfoNtf*>(msg.get());

    auto it = agents_.find(ntf->server_info().server_type());
    if (it == agents_.end()) {
        ERROR_LOG("agent not exist\n");
        return;
    }

    it->second->setStub(ntf->server_info());
}

void AgentManager::rmSvrHandle(int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    pb::RemoveServerNtf *ntf = static_cast<pb::RemoveServerNtf*>(msg.get());

    auto it = agents_.find(ntf->server_type());
    if (it == agents_.end()) {
        ERROR_LOG("agent not exist\n");
        return;
    }

    it->second->removeStub(ntf->server_id());
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

    return nullptr;
}

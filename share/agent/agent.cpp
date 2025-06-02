/*
 * Created by Xianke Liu on 2024/7/16.
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

#include "agent.h"
#include "agent_manager.h"

using namespace wukong;

Agent::~Agent() {}

void Agent::resetStubs(const std::list<pb::ServerInfo> &serverInfos) {
    std::list<ServerId> removeServerIds;
    std::set<ServerId> addedServerIds;

    for (const auto &serverInfo : serverInfos) {
        addedServerIds.insert(serverInfo.server_id());

        setStub(serverInfo);
    }

    for (auto &pair : stubInfos_) {
        if (addedServerIds.find(pair.first) == addedServerIds.end()) {
            removeServerIds.push_back(pair.first);
        }
    }

    for (auto serverId : removeServerIds) {
        removeStub(serverId);
    }
}

void Agent::removeStub(ServerId sid) {
    stubInfos_.erase(sid);
}

std::shared_ptr<google::protobuf::Service> Agent::getStub(ServerId sid) {
    auto it = stubInfos_.find(sid);

    if (it == stubInfos_.end()) {
        return nullptr;
    }

    return it->second.stub;
}

pb::ServerInfo* Agent::getServerInfo(ServerId sid) {
    auto it = stubInfos_.find(sid);

    if (it == stubInfos_.end()) {
        return nullptr;
    }

    return &it->second.info;
}

bool Agent::randomServer(ServerId &serverId) {
    if (stubInfos_.empty()) {
        return false;
    }

    auto it = stubInfos_.begin();
    if (stubInfos_.size() > 1) {
        std::advance(it, rand() % stubInfos_.size());
    }
    
    serverId = it->first;
    return true;
}

GameAgent::~GameAgent() {}

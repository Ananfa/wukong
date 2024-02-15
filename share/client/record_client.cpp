/*
 * Created by Xianke Liu on 2020/1/15.
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

#include "record_client.h"
#include "corpc_controller.h"
#include "string_utils.h"

using namespace corpc;
using namespace wukong;

std::map<std::string, std::shared_ptr<pb::RecordService_Stub>> RecordClient::addr2stubs_;
std::map<ServerId, RecordClient::StubInfo> RecordClient::stubs_;
Mutex RecordClient::stubsLock_;
std::atomic<uint32_t> RecordClient::stubChangeNum_(0);
thread_local uint32_t RecordClient::t_stubChangeNum_ = 0;
thread_local std::map<ServerId, RecordClient::StubInfo> RecordClient::t_stubs_;

std::vector<RecordClient::ServerInfo> RecordClient::getServerInfos() {
    std::vector<ServerInfo> infos;

    // 对相同rpc地址的服务不需要重复获取
    std::map<std::string, bool> handledAddrs; // 记录已处理的rpc地址

    refreshStubs();
    std::map<ServerId, StubInfo> localStubs = t_stubs_;
    
    // 清除原来的信息
    infos.reserve(localStubs.size());
    // 利用(总在线人数 - 服务器在线人数)作为分配服务器的权重
    uint32_t totalCount = 0;
    for (auto &kv : localStubs) {
        if (handledAddrs.find(kv.second.rpcAddr) != handledAddrs.end()) {
            continue;
        }

        handledAddrs.insert(std::make_pair(kv.second.rpcAddr, true));

        corpc::Void *request = new corpc::Void();
        pb::OnlineCounts *response = new pb::OnlineCounts();
        Controller *controller = new Controller();
        kv.second.stub->getOnlineCount(controller, request, response, nullptr);
        
        if (controller->Failed()) {
            ERROR_LOG("LobbyClient::getServerInfos -- Rpc Call Failed : %s\n", controller->ErrorText().c_str());
            continue;
        } else {
            auto countInfos = response->counts();

            for (auto it = countInfos.begin(); it != countInfos.end(); ++it) {
                infos.push_back({it->serverid(), it->count(), 0});
                totalCount += it->count();
            }
        }
        
        delete controller;
        delete response;
        delete request;
    }
    
    for (auto &info : infos) {
        info.weight = totalCount - info.count;
    }

    return infos;
}

bool RecordClient::loadRoleData(ServerId sid, RoleId roleId, UserId &userId, const std::string &lToken, ServerId &serverId, std::string &roleData) {
    std::shared_ptr<pb::RecordService_Stub> stub = getStub(sid);
    
    if (!stub) {
        ERROR_LOG("RecordClient::loadRoleData -- record server %d stub not avaliable, waiting.\n", sid);
        return false;
    }

    pb::LoadRoleDataRequest *request = new pb::LoadRoleDataRequest();
    pb::LoadRoleDataResponse *response = new pb::LoadRoleDataResponse();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_ltoken(lToken);
    request->set_roleid(roleId);
    stub->loadRoleData(controller, request, response, nullptr);

    bool result = false;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else if (response->errcode() == 0) {
        result = true;
        serverId = response->serverid();
        userId = response->userid();
        roleData = response->data();
    }

    delete controller;
    delete response;
    delete request;

    return result;
}

bool RecordClient::setServers(const std::vector<AddressInfo>& addresses) {
    if (!client_) {
        ERROR_LOG("RecordClient::setServers -- not init\n");
        return false;
    }

    std::map<std::string, std::shared_ptr<pb::RecordService_Stub>> addr2stubs;
    std::map<ServerId, StubInfo> stubs;
    for (const auto& address : addresses) {
        std::shared_ptr<pb::RecordService_Stub> stub;
        std::string addr = address.ip + std::to_string(address.port);
        auto iter = addr2stubs.find(addr);
        if (iter != addr2stubs.end()) {
            ERROR_LOG("RecordClient::setServers -- duplicate rpc addr: %s\n", addr.c_str());
            continue;
        }

        iter = addr2stubs_.find(addr);
        if (iter != addr2stubs_.end()) {
            stub = iter->second;
        } else {
            stub = std::make_shared<pb::RecordService_Stub>(new RpcClient::Channel(client_, address.ip, address.port, 1), pb::RecordService::STUB_OWNS_CHANNEL);
        }
        addr2stubs.insert(std::make_pair(addr, stub));

        for (uint16_t serverId : address.serverIds) {
            auto iter1 = stubs_.find(serverId);
            if (iter1 != stubs_.end()) {
                if (iter1->second.rpcAddr == addr) {
                    stubs.insert(std::make_pair(serverId, iter1->second));
                    continue;
                }
            }

            StubInfo stubInfo = {
                addr,
                stub
            };
            stubs.insert(std::make_pair(serverId, std::move(stubInfo)));
        }
    }

    addr2stubs_ = addr2stubs;
    
    {
        LockGuard lock(stubsLock_);
        stubs_ = stubs;
        stubChangeNum_++;
    }
    
    return true;
}

std::shared_ptr<pb::RecordService_Stub> RecordClient::getStub(ServerId sid) {
    refreshStubs();
    auto iter = t_stubs_.find(sid);
    if (iter == t_stubs_.end()) {
        return nullptr;
    }

    return iter->second.stub;
}

void RecordClient::refreshStubs() {
    if (t_stubChangeNum_ != stubChangeNum_) {
        {
            LockGuard lock(stubsLock_);

            if (t_stubChangeNum_ == stubChangeNum_) {
                return;
            }

            t_stubs_.clear();
            t_stubs_ = stubs_;
            t_stubChangeNum_ = stubChangeNum_;
        }
    }
}

bool RecordClient::parseAddress(const std::string &input, AddressInfo &addressInfo) {
    std::vector<std::string> output;
    StringUtils::split(input, "|", output);

    int outputSize = output.size();
    if (outputSize < 2) return false;

    std::vector<std::string> output1;
    StringUtils::split(output[0], ":", output1);
    if (output1.size() != 2) return false;
    addressInfo.ip = output1[0];
    addressInfo.port = std::stoi(output1[1]);
    addressInfo.serverIds.reserve(outputSize-1);
    for (int i = 1; i < outputSize; i++) {
        addressInfo.serverIds.push_back(std::stoi(output[i]));
    }

    return true;
}
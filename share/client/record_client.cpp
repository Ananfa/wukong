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

using namespace wukong;

std::map<std::string, std::shared_ptr<pb::RecordService_Stub>> RecordClient::_addr2stubs;
std::map<ServerId, RecordClient::StubInfo> RecordClient::_stubs;
std::mutex RecordClient::_stubsLock;
std::atomic<uint32_t> RecordClient::_stubChangeNum(0);
thread_local uint32_t RecordClient::_t_stubChangeNum = 0;
thread_local std::map<ServerId, RecordClient::StubInfo> RecordClient::_t_stubs;

std::vector<RecordClient::ServerInfo> RecordClient::getServerInfos() {
    std::vector<ServerInfo> infos;

    // 对相同rpc地址的服务不需要重复获取
    std::map<std::string, bool> handledAddrs; // 记录已处理的rpc地址

    refreshStubs();
    std::map<ServerId, StubInfo> localStubs = _t_stubs;
    
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

bool RecordClient::loadRole(ServerId sid, RoleId roleId, uint32_t lToken, ServerId &serverId, std::string &roleData) {
    std::shared_ptr<pb::RecordService_Stub> stub = getStub(sid);
    
    if (!stub) {
        ERROR_LOG("RecordClient::loadRole -- record server %d stub not avaliable, waiting.\n", sid);
        return false;
    }

    pb::LoadRoleRequest *request = new pb::LoadRoleRequest();
    pb::LoadRoleResponse *response = new pb::LoadRoleResponse();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_ltoken(lToken);
    request->set_roleid(roleId);
    stub->loadRole(controller, request, response, nullptr);

    bool result = false;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else if (response->errcode() == 0) {
        result = true;
        serverId = response->serverid();
        roleData = response->data();
    }

    delete controller;
    delete response;
    delete request;

    return result;
}

bool RecordClient::setServers(const std::vector<AddressInfo>& addresses) {
    if (!_client) {
        ERROR_LOG("RecordClient::setServers -- not init\n");
        return false;
    }

    std::map<std::string, std::shared_ptr<pb::RecordService_Stub>> addr2stubs;
    std::map<ServerId, StubInfo> stubs;
    for (const auto& kv : addresses) {
        std::shared_ptr<pb::RecordService_Stub> stub;
        std::string addr = address.ip + std::to_string(address.port);
        auto iter = addr2stubs.find(addr);
        if (iter != addr2stubs.end()) {
            ERROR_LOG("RecordClient::setServers -- duplicate rpc addr: %s\n", addr.c_str());
            continue;
        }

        iter = _addr2stubs.find(addr);
        if (iter != _addr2stubs.end()) {
            stub = iter->second;
        } else {
            stub = std::make_shared<pb::RecordService_Stub>(new RpcClient::Channel(_client, address.ip, address.port, 1), pb::RecordService::STUB_OWNS_CHANNEL);
        }
        addr2stubs.insert(std::make_pair(addr, stub));

        for (uint16_t serverId : address.serverIds) {
            auto iter1 = _stubs.find(serverId);
            if (iter1 != _stubs.end()) {
                if (iter1->second.rpcAddr == addr) {
                    stubs.insert(std::make_pair(serverId, iter1->second));
                    continue;
                }
            }

            StubInfo stubInfo = {
                addr,
                stub
            };
            stubs.insert(std::make_pair(pair.first, std::move(stubInfo)));
        }
    }

    _addr2stubs = addr2stubs;
    
    {
        std::unique_lock<std::mutex> lock(_stubsLock);
        _stubs = stubs;
        _stubChangeNum++;
    }
    
    return true;
}

std::shared_ptr<pb::RecordService_Stub> RecordClient::getStub(ServerId sid) {
    refreshStubs();
    auto iter = _t_stubs.find(sid);
    if (iter == _t_stubs.end()) {
        return nullptr;
    }

    return iter->second.stub;
}

void RecordClient::refreshStubs() {
    if (_t_stubChangeNum != _stubChangeNum) {
        _t_stubs.clear();
        {
            std::unique_lock<std::mutex> lock(_stubsLock);
            _t_stubs = _stubs;
            _t_stubChangeNum = _stubChangeNum;
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
    addressInfo.serverPorts.reserve(outputSize-1);
    for (int i = 1; i < outputSize; i++) {
        addressInfo.serverIds.push_back(std::stoi(output[i]));
    }

    return true;
}
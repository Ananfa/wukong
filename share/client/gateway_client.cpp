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

#include "gateway_client.h"
#include "corpc_controller.h"
#include "string_utils.h"

using namespace wukong;

std::map<ServerId, GatewayClient::StubInfo> GatewayClient::_stubs;
std::mutex GatewayClient::_stubsLock;
std::atomic<uint32_t> GatewayClient::_stubChangeNum(0);
thread_local uint32_t GatewayClient::_t_stubChangeNum = 0;
thread_local std::map<ServerId, GatewayClient::StubInfo> GatewayClient::_t_stubs;

static void callDoneHandle(::google::protobuf::Message *request, Controller *controller) {
    delete controller;
    delete request;
}

void GatewayClient::shutdown() {
    refreshStubs();
    std::map<ServerId, GatewayClient::StubInfo> stubs = _t_stubs;
    
    for (const auto& kv : stubs) {
        corpc::Void *request = new corpc::Void();
        Controller *controller = new Controller();
        
        kv.second.stub->shutdown(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
    }
}

bool GatewayClient::kick(ServerId gatewayId, UserId userId) {
    std::shared_ptr<pb::GatewayService_Stub> stub = getStub(gatewayId);
    
    if (!stub) {
        ERROR_LOG("GatewayClient server %d stub not avaliable, waiting.\n", gatewayId);
        return false;
    }

    bool result = false;
    pb::Uint32Value *request = new pb::Uint32Value();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_value(userId);
    stub->kick(controller, request, response, nullptr);

    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
    } else {
        result = response->value();
    }

    delete controller;
    delete response;
    delete request;

    return result;
}

std::vector<GatewayClient::ServerInfo> GatewayClient::getServerInfos() {
    std::vector<ServerInfo> infos;

    refreshStubs();
    std::map<uint16_t, StubInfo> localStubs = _t_stubs;
    
    // 清除原来的信息
    infos.reserve(localStubs.size());
    // 利用(总在线人数 - 服务器在线人数)作为分配服务器的权重
    uint32_t totalCount = 0;
    for (auto &kv : localStubs) {
        corpc::Void *request = new corpc::Void();
        pb::Uint32Value *response = new pb::Uint32Value();
        Controller *controller = new Controller();
        kv.second.stub->getOnlineCount(controller, request, response, nullptr);
        
        if (controller->Failed()) {
            ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
            continue;
        } else {
            infos.push_back({kv.first, kv.second.outerAddr, kv.second.outerPort, response->value(), 0});
            totalCount += response->value();
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

void GatewayClient::forwardOut(ServerId gatewayId, int32_t type, const std::vector<std::pair<UserId, RoleId>> &targets, const std::string &msg) {
    std::shared_ptr<pb::GatewayService_Stub> stub = getStub(gatewayId);
    
    if (!stub) {
        ERROR_LOG("GatewayClient server %d stub not avaliable, waiting.\n", gatewayId);
        return;
    }
    
    pb::ForwardRequest *request = new pb::ForwardRequest();
    Controller *controller = new Controller();
    request->set_type(type);
    for (auto it = targets.begin(); it != targets.end(); it++) {
        ::wukong::pb::ForwardOutTarget* target = request->add_targets();
        target->set_userid(it->first);
        target->set_roleid(it->second);
    }
    
    if (!msg.empty()) {
        request->set_rawmsg(msg);
    }
    
    stub->forwardOut(controller, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(&callDoneHandle, request, controller));
}

bool GatewayClient::heartbeat(ServerId gatewayId, UserId userId, RoleId roleId, GameServerType stype, ServerId sid) {
    pb::HeartbeatRequest *request = new pb::HeartbeatRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    kv.second.stub->heartbeat(controller, request, response, nullptr);
    
    bool ret;
    if (controller->Failed()) {
        ERROR_LOG("Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        ret = false;
    } else {
        ret = response->value();
    }
    
    delete controller;
    delete response;
    delete request;

    return ret;
}

bool GatewayClient::setServers(const std::map<ServerId, AddressInfo>& addresses) {
    if (!_client) {
        ERROR_LOG("GatewayClient not init\n");
        return false;
    }
    
    std::map<ServerId, StubInfo> stubs;
    for (const auto& kv : addresses) {
        ServerId sid = kv.first;
        const AddressInfo& address = kv.second;
        auto iter1 = _stubs.find(sid);
        if (iter1 != _stubs.end()) {
            if (iter1->second.ip == address.ip && iter1->second.port == address.port) {
                stubs.insert(std::make_pair(sid, iter1->second));
                continue;
            }
        }

        StubInfo stubInfo = {
            address.ip,
            address.port,
            address.outerAddr,
            address.outerPort,
            std::make_shared<pb::GatewayService_Stub>(new RpcClient::Channel(_client, address.ip, address.port, 1), pb::GatewayService::STUB_OWNS_CHANNEL)
        };
        stubs.insert(std::make_pair(sid, std::move(stubInfo)));
    }
    
    {
        std::unique_lock<std::mutex> lock(_stubsLock);
        _stubs = stubs;
        _stubChangeNum++;
    }
    
    return true;
}

std::shared_ptr<pb::GatewayService_Stub> GatewayClient::getStub(ServerId sid) {
    refreshStubs();
    auto iter = _t_stubs.find(sid);
    if (iter == _t_stubs.end()) {
        return nullptr;
    }
    
    return iter->second.stub;
}

void GatewayClient::refreshStubs() {
    if (_t_stubChangeNum != _stubChangeNum) {
        _t_stubs.clear();
        {
            std::unique_lock<std::mutex> lock(_stubsLock);
            _t_stubs = _stubs;
            _t_stubChangeNum = _stubChangeNum;
        }
    }
}

bool GatewayClient::parseAddress(const std::string &input, AddressInfo &addressInfo) {
    std::vector<std::string> output;
    StringUtils::split(input, "|", output);
    
    if (output.size() != 3) return false;

    std::vector<std::string> output1;
    StringUtils::split(output[1], ":", output1);
    if (output1.size() != 2) return false;

    std::vector<std::string> output2;
    StringUtils::split(output[2], ":", output2);
    if (output2.size() != 2) return false;

    addressInfo.id = std::stoi(output[0]);
    addressInfo.ip = output1[0];
    addressInfo.port = std::stoi(output1[1]);
    addressInfo.outerAddr = output2[0];
    addressInfo.outerPort = std::stoi(output2[1]);

    return true;
}

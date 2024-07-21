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

using namespace corpc;
using namespace wukong;

std::map<std::string, std::shared_ptr<pb::GatewayService_Stub>> GatewayClient::addr2stubs_;
std::map<ServerId, GatewayClient::StubInfo> GatewayClient::stubs_;
Mutex GatewayClient::stubsLock_;
std::atomic<uint32_t> GatewayClient::stubChangeNum_(0);
thread_local uint32_t GatewayClient::t_stubChangeNum_ = 0;
thread_local std::map<ServerId, GatewayClient::StubInfo> GatewayClient::t_stubs_;

void GatewayClient::shutdown() {
    refreshStubs();
    std::map<ServerId, GatewayClient::StubInfo> stubs = t_stubs_;
    
    for (const auto& kv : stubs) {
        corpc::Void *request = new corpc::Void();
        
        kv.second.stub->shutdown(nullptr, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request));
    }
}

bool GatewayClient::kick(ServerId sid, UserId userId, const std::string &gToken) {
    std::shared_ptr<pb::GatewayService_Stub> stub = getStub(sid);
    
    if (!stub) {
        ERROR_LOG("GatewayClient::kick -- server %d stub not avaliable, waiting.\n", sid);
        return false;
    }

    bool result = false;
    pb::KickRequest *request = new pb::KickRequest();
    pb::BoolValue *response = new pb::BoolValue();
    Controller *controller = new Controller();
    request->set_serverid(sid);
    request->set_userid(userId);
    request->set_gtoken(gToken);
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

    // 对相同rpc地址的服务不需要重复获取
    std::map<std::string, bool> handledAddrs; // 记录已处理的rpc地址

    refreshStubs();
    std::map<uint16_t, StubInfo> localStubs = t_stubs_;

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
        pb::OnlineCount *response = new pb::OnlineCount();
        Controller *controller = new Controller();
        kv.second.stub->getOnlineCount(controller, request, response, nullptr);
        
        if (controller->Failed()) {
            ERROR_LOG("GatewayClient::getServerInfos -- Rpc Call Failed : %s\n", controller->ErrorText().c_str());
        } else {
            infos.push_back({kv.first, kv.second.outerAddr, kv.second.outerPort, response->count(), 0});
            totalCount += response->count();
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

void GatewayClient::broadcast(ServerId sid, int32_t type, uint16_t tag, const std::vector<std::pair<UserId, std::string>> &targets, const std::string &msg) {
    if (sid == 0) { // 全服广播
        if (targets.size() > 0) { // 全服广播不应设置指定目标
            WARN_LOG("GatewayClient::broadcast -- targets list not empty when global broadcast\n");
        }

        // 全服广播
        refreshStubs();
        std::map<uint16_t, StubInfo> localStubs = t_stubs_;

        if (localStubs.size() > 0) {
            pb::ForwardOutRequest *request = new pb::ForwardOutRequest();
            request->set_serverid(sid);
            request->set_type(type);
            request->set_tag(tag);
            for (auto &t : targets) {
                ::wukong::pb::ForwardOutTarget* target = request->add_targets();

                target->set_userid(t.first);
                target->set_ltoken(t.second);
            }
            
            if (!msg.empty()) {
                request->set_rawmsg(msg);
            }

            std::shared_ptr<::google::protobuf::Closure> donePtr(google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request), [](::google::protobuf::Closure *done) {
                done->Run();
            });

            // 对相同rpc地址的服务不需要重复获取
            std::map<std::string, bool> handledAddrs; // 记录已处理的rpc地址
            for (auto &kv : localStubs) {
                if (handledAddrs.find(kv.second.rpcAddr) != handledAddrs.end()) {
                    continue;
                }

                handledAddrs.insert(std::make_pair(kv.second.rpcAddr, true));

                kv.second.stub->forwardOut(nullptr, request, nullptr, google::protobuf::NewCallback(callDoneHandle, donePtr));
            }
        }
    } else { // 单服广播或多播
        std::shared_ptr<pb::GatewayService_Stub> stub = getStub(sid);
        
        if (!stub) {
            ERROR_LOG("GatewayClient::broadcast -- server %d stub not avaliable, waiting.\n", sid);
            return;
        }
        
        pb::ForwardOutRequest *request = new pb::ForwardOutRequest();
        request->set_serverid(sid);
        request->set_type(type);
        request->set_tag(tag);
        for (auto &t : targets) {
            ::wukong::pb::ForwardOutTarget* target = request->add_targets();

            target->set_userid(t.first);
            target->set_ltoken(t.second);
        }
        
        if (!msg.empty()) {
            request->set_rawmsg(msg);
        }
        
        stub->forwardOut(nullptr, request, nullptr, google::protobuf::NewCallback<::google::protobuf::Message *>(callDoneHandle, request));  
    }
}

bool GatewayClient::setServers(const std::vector<AddressInfo>& addresses) {
    if (!client_) {
        FATAL_LOG("GatewayClient::setServers -- not init\n");
        return false;
    }

    std::map<std::string, std::shared_ptr<pb::GatewayService_Stub>> addr2stubs;
    std::map<ServerId, StubInfo> stubs;
    for (const auto& address : addresses) {
        std::shared_ptr<pb::GatewayService_Stub> stub;
        std::string addr = address.ip + std::to_string(address.port);
        auto iter = addr2stubs.find(addr);
        if (iter != addr2stubs.end()) {
            ERROR_LOG("GatewayClient::setServers -- duplicate rpc addr: %s\n", addr.c_str());
            continue;
        }

        iter = addr2stubs_.find(addr);
        if (iter != addr2stubs_.end()) {
            stub = iter->second;
        } else {
            stub = std::make_shared<pb::GatewayService_Stub>(new RpcClient::Channel(client_, address.ip, address.port, 1), pb::GatewayService::STUB_OWNS_CHANNEL);
        }
        addr2stubs.insert(std::make_pair(addr, stub));

        for (const auto& pair : address.serverPorts) {
            auto iter1 = stubs_.find(pair.first);
            if (iter1 != stubs_.end()) {
                if (iter1->second.rpcAddr == addr) {
                    stubs.insert(std::make_pair(pair.first, iter1->second));
                    continue;
                }
            }

            StubInfo stubInfo = {
                addr,
                address.outerAddr,
                pair.second,
                stub
            };
            stubs.insert(std::make_pair(pair.first, std::move(stubInfo)));
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

std::shared_ptr<pb::GatewayService_Stub> GatewayClient::getStub(ServerId sid) {
    refreshStubs();
    auto iter = t_stubs_.find(sid);
    if (iter == t_stubs_.end()) {
        return nullptr;
    }
    
    return iter->second.stub;
}

void GatewayClient::refreshStubs() {
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

bool GatewayClient::parseAddress(const std::string &input, AddressInfo &addressInfo) {
    std::vector<std::string> output;
    StringUtils::split(input, "|", output);

    int outputSize = output.size();
    if (outputSize < 2) return false;

    std::vector<std::string> output1;
    StringUtils::split(output[0], ":", output1);
    if (output1.size() != 3) return false;
    addressInfo.ip = output1[0];
    addressInfo.port = std::stoi(output1[1]);
    addressInfo.outerAddr = output1[2];
    addressInfo.serverPorts.reserve(outputSize-1);
    for (int i = 1; i < outputSize; i++) {
        std::vector<std::string> output2;
        StringUtils::split(output[i], ":", output2);
        if (output2.size() != 2) return false;
        addressInfo.serverPorts.push_back(std::make_pair(std::stoi(output2[0]), std::stoi(output2[1])));
    }

    return true;
}

/*
 * Created by Xianke Liu on 2026/3/25.
 */

#include "battle_agent.h"

#include "corpc_controller.h"

using namespace corpc;
using namespace wukong;

void BattleAgent::setStub(const pb::ServerInfo &serverInfo) {
    auto it = stubInfos_.find(serverInfo.server_id());
    if (it != stubInfos_.end()) {
        if (serverInfo.rpc_host() == it->second.info.rpc_host() && serverInfo.rpc_port() == it->second.info.rpc_port()) {
            it->second.info = serverInfo;
            return;
        }

        it->second.info = serverInfo;

        if (client_) {
            it->second.stub = std::make_shared<pb::BattleService_Stub>(
                new RpcClient::Channel(client_, serverInfo.rpc_host(), serverInfo.rpc_port(), 1),
                google::protobuf::Service::STUB_OWNS_CHANNEL);
        }

        return;
    }

    if (client_) {
        stubInfos_.insert(std::make_pair(
            serverInfo.server_id(),
            StubInfo{serverInfo, std::make_shared<pb::BattleService_Stub>(
                                     new RpcClient::Channel(client_, serverInfo.rpc_host(), serverInfo.rpc_port(), 1),
                                     google::protobuf::Service::STUB_OWNS_CHANNEL)}));
    } else {
        stubInfos_.insert(std::make_pair(serverInfo.server_id(), StubInfo{serverInfo, nullptr}));
    }
}

void BattleAgent::shutdown() {
    // BattleService 当前未定义 shutdown RPC，保留空实现以统一 Agent 接口
}

bool BattleAgent::requestBattleAssignment(ServerId sid, const pb::RequestBattleAssignmentRequest &req,
                                          pb::RequestBattleAssignmentResponse *resp) {
    if (!resp) {
        return false;
    }
    auto it = stubInfos_.find(sid);
    if (it == stubInfos_.end() || !it->second.stub) {
        ERROR_LOG("BattleAgent::requestBattleAssignment -- battle stub %u not ready\n", sid);
        return false;
    }
    auto stub = std::static_pointer_cast<pb::BattleService_Stub>(it->second.stub);
    Controller ctl;
    stub->requestBattleAssignment(&ctl, &req, resp, nullptr);
    if (ctl.Failed()) {
        ERROR_LOG("BattleAgent::requestBattleAssignment -- rpc failed: %s\n", ctl.ErrorText().c_str());
        return false;
    }
    return true;
}

void BattleAgent::removeBattlePlayer(ServerId sid, const pb::RemoveBattlePlayerRequest &req) {
    auto it = stubInfos_.find(sid);
    if (it == stubInfos_.end() || !it->second.stub) {
        ERROR_LOG("BattleAgent::removeBattlePlayer -- battle stub %u not ready\n", sid);
        return;
    }
    auto stub = std::static_pointer_cast<pb::BattleService_Stub>(it->second.stub);
    auto *request = new pb::RemoveBattlePlayerRequest();
    request->CopyFrom(req);
    auto *controller = new Controller();
    stub->removeBattlePlayer(controller, request, nullptr,
                             google::protobuf::NewCallback<google::protobuf::Message *, Controller *>(callDoneHandle, request, controller));
}

/*
 * Created by Xianke Liu on 2024/7/5.
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

#include "nexus_handler.h"
#include "corpc_define.h"
#include "nexus_config.h"
#include "server_manager.h"
#include "share/const.h"

#include "inner_common.pb.h"

using namespace wukong;

void NexusHandler::registerMessages(corpc::MessageTerminal *terminal) {
    terminal->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false, NexusHandler::connectHandle);
    terminal->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, false, NexusHandler::closeHandle);
    terminal->registerMessage(S2N_MESSAGE_ID_ACCESS, nullptr, false, NexusHandler::accessHandle);
    terminal->registerMessage(S2N_MESSAGE_ID_UPDATE, nullptr, false, NexusHandler::updateHandle);
}

void NexusHandler::connectHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    // 登记未认证连接
    DEBUG_LOG("NexusHandler::connectHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());

    if (conn->isOpen()) {
        g_ServerManager.addUnaccessConn(conn);
    } else {
        DEBUG_LOG("NexusHandler::connectHandle -- conn %d[%d] is not open\n", conn.get(), conn->getfd());
    }
}

void NexusHandler::closeHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    DEBUG_LOG("NexusHandler::closeHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());
    assert(!conn->isOpen());

    if (g_ServerManager.isUnaccess(conn)) {
        g_ServerManager.removeUnaccessConn(conn);
    } else {
        auto sobj = g_ServerManager.getConnectedServerObject(conn);
        if (!sobj) {
            ERROR_LOG("NexusHandler::closeHandle -- unknown conn\n");
            return;
        }

        g_ServerManager.moveToDisconnected(sobj);
    }
}

void NexusHandler::accessHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    DEBUG_LOG("NexusHandler::accessHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());

    if (!conn->isOpen()) {
        ERROR_LOG("NexusHandler::accessHandle -- connection closed when access handle\n");
        return;
    }

    // 确认连接是未接入连接（不允许重复发接入消息）
    if (!g_ServerManager.isUnaccess(conn)) {
        ERROR_LOG("NexusHandler::accessHandle -- not an unaccess connection\n");
        conn->close();
        return;
    }

    g_ServerManager.removeUnaccessConn(conn);

    pb::ServerAccessRequest *request = static_cast<pb::ServerAccessRequest*>(msg.get());
    
    // 重复判断
    ServerType stype = (ServerType)request->server_info().server_type();
    ServerId sid = (ServerId)request->server_info().server_id();

    // 对断线的ServerObject进行连接恢复
    auto sobj = g_ServerManager.getServerObject(stype, sid);
    if (sobj) {
        if (sobj->getConn()) {
            ERROR_LOG("NexusHandler::accessHandle -- stype:%d sid:%d repeat access\n", stype, sid);
            conn->close();
            return;
        }
    } else {
        sobj = std::make_shared<ServerObject>(request->server_info());
    }

    sobj->setConn(conn);
    g_ServerManager.addConnectedServer(sobj);

    // 向新接入的服务器发其关注的服务信息
    std::shared_ptr<pb::ServerAccessResponse> response(new pb::ServerAccessResponse);
    auto& concerns = g_NexusConfig.getConcerns(stype);
    for (auto concernType : concerns) {
        auto &objMap = g_ServerManager.getServerObjects(concernType);
        for (auto it : objMap) {
            auto sinfo = response->add_server_infos();
            *sinfo = it.second->getInfo();
        }
    }

    sobj->send(N2S_MESSAGE_ID_ACCESS_RSP, response);

    // 向关注新接入服务器的服务器发新服信息
    auto& beConcerns = g_NexusConfig.getBeConcerns(stype);
    if (!beConcerns.empty()) {
        std::shared_ptr<pb::ServerInfoNtf> ntf(new pb::ServerInfoNtf);
        auto ntfInfo = ntf->mutable_server_info();
        *ntfInfo = sobj->getInfo();

        for (auto beConcernType : beConcerns) {
            auto &objMap = g_ServerManager.getServerObjects(beConcernType);
            for (auto it : objMap) {
                it.second->send(N2S_MESSAGE_ID_SVRINFO, ntf);
            }
        }
    }
}

void NexusHandler::updateHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    DEBUG_LOG("NexusHandler::updateHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());

    auto sobj = g_ServerManager.getConnectedServerObject(conn);
    if (!sobj) {
        WARN_LOG("NexusHandler::updateHandle -- cant find connected server object\n");
        return;
    }
    ServerType stype = sobj->getType();
    ServerId sid = sobj->getId();

    pb::ServerInfoNtf *ntf = static_cast<pb::ServerInfoNtf*>(msg.get());
    if (stype != ntf->server_info().server_type() || sid != ntf->server_info().server_id()) {
        ERROR_LOG("NexusHandler::updateHandle -- server type or server id changed\n");
        return;
    }

    sobj->setInfo(ntf->server_info());

    // 向关注的服务器发服务更新信息
    auto& beConcerns = g_NexusConfig.getBeConcerns(stype);
    if (!beConcerns.empty()) {
        for (auto beConcernType : beConcerns) {
            auto &objMap = g_ServerManager.getServerObjects(beConcernType);
            for (auto it : objMap) {
                it.second->send(N2S_MESSAGE_ID_SVRINFO, msg);
            }
        }
    }

}

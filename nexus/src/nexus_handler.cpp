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
#include "server_manager.h"
#include "share/const.h"

#include "inner_common.pb.h"

using namespace wukong;

void NexusHandler::registerMessages(corpc::MessageTerminal *terminal) {
    terminal->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false, NexusHandler::connectHandle);
    terminal->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, true, NexusHandler::closeHandle);
    terminal->registerMessage(S2N_MESSAGE_ID_ACCESS, nullptr, true, NexusHandler::accessHandle);
}

void NexusHandler::connectHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    // 登记未认证连接
    DEBUG_LOG("NexusHandler::connectHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());

    if (conn->isOpen()) {
        g_ServerManager.addUnknownConn(conn);
    } else {
        DEBUG_LOG("NexusHandler::connectHandle -- conn %d[%d] is not open\n", conn.get(), conn->getfd());
    }
}

void NexusHandler::closeHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    DEBUG_LOG("NexusHandler::closeHandle -- conn:%d[%d]\n", conn.get(), conn->getfd());
    assert(!conn->isOpen());

    if (g_ServerManager.isUnknown(conn)) {
        g_ServerManager.removeUnknownConn(conn);
    } else {
        g_ServerManager.tryMoveToDisconnectedLink(conn);
    }
}

void NexusHandler::accessHandle(int16_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg, std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
    
}
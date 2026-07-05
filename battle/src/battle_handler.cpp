/*
 * Created by Xianke Liu on 2025/12/26.
 */

#include "battle_handler.h"
#include "battle_const.h"
#include "battle_role_manager.h"

#include "corpc_message_server.h"
#include "corpc_message_buffer.h"
#include "battle_sync.pb.h"
#include "corpc_utils.h"

namespace wukong {

void BattleHandler::registerMessages(corpc::KcpMessageTerminal *terminal) {
    terminal->registerMessage(CORPC_MSG_TYPE_CONNECT, nullptr, false,
        [](int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg,
           std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
            (void)type;
            (void)tag;
            (void)msg;
            //std::shared_ptr<corpc::MessageBuffer> buf(new corpc::MessageBuffer(true));
            //conn->setMsgBuffer(buf);
            g_BattleRoleManager.onConnectionOpened(conn);
            DEBUG_LOG("BattleHandler KCP connect fd:%d\n", conn->getfd());
        });

    terminal->registerMessage(CORPC_MSG_TYPE_CLOSE, nullptr, false,
        [](int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg,
           std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
            (void)type;
            (void)tag;
            (void)msg;
            g_BattleRoleManager.onConnectionClosed(conn);
            DEBUG_LOG("BattleHandler KCP close fd:%d\n", conn->getfd());
        });

    terminal->registerMessage(BATTLE_KCP_MSG_AUTH, new pb::BattleKcpAuth, false,
        [](int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg,
           std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
            (void)type;
            (void)tag;
            auto auth = std::dynamic_pointer_cast<pb::BattleKcpAuth>(msg);
            if (!auth) {
                return;
            }
            if (!g_BattleRoleManager.onKcpAuth(auth, conn)) {
                WARN_LOG("BattleHandler -- kcp auth failed room:%llu role:%llu\n",
                    (unsigned long long)auth->room_id(), (unsigned long long)auth->role_id());
            }
        });

    terminal->registerMessage(BATTLE_KCP_MSG_LEAVE_ROOM, new pb::BattleKcpLeaveRoom, false,
        [](int32_t type, uint16_t tag, std::shared_ptr<google::protobuf::Message> msg,
           std::shared_ptr<corpc::MessageTerminal::Connection> conn) {
            (void)type;
            (void)tag;
            (void)conn;
            auto leave = std::dynamic_pointer_cast<pb::BattleKcpLeaveRoom>(msg);
            if (!leave) {
                return;
            }
            if (!g_BattleRoleManager.onKcpLeaveRoom(leave)) {
                WARN_LOG("BattleHandler -- leave room failed room:%llu role:%llu\n",
                    (unsigned long long)leave->room_id(), (unsigned long long)leave->role_id());
            }
        });
}

}

/*
 * Created by Xianke Liu on 2026/3/26.
 */

#include "battle_role_manager.h"
#include "battle_room_manager.h"

#include "corpc_routine_env.h"
#include "corpc_utils.h"

#include <unistd.h>

namespace wukong {

namespace {
uintptr_t connKey(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    return reinterpret_cast<uintptr_t>(conn.get());
}
}

void BattleRoleManager::init() {
    RoutineEnvironment::startCoroutine(cleanupRoutine, nullptr);
}

void *BattleRoleManager::cleanupRoutine(void *arg) {
    (void)arg;
    while (true) {
        sleep(1);
        g_BattleRoleManager.cleanupUnauthedConnections(std::time(nullptr), kConnAuthTimeoutSec);
    }
    return nullptr;
}

void BattleRoleManager::removeUnauthedConn(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn) {
        return;
    }
    auto it = pendingAuthNodeMap_.find(conn.get());
    if (it == pendingAuthNodeMap_.end()) {
        return;
    }
    pendingAuthLink_.erase(it->second);
    pendingAuthNodeMap_.erase(it);
}

void BattleRoleManager::onConnectionOpened(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn) {
        return;
    }
    assert(pendingAuthNodeMap_.find(conn.get()) == pendingAuthNodeMap_.end());
    PendingAuthNode *node = new PendingAuthNode();
    node->data = conn;
    pendingAuthLink_.push(node);
    pendingAuthNodeMap_[conn.get()] = node;
}

void BattleRoleManager::onConnectionClosed(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn) {
        return;
    }
    const uintptr_t key = connKey(conn);
    removeUnauthedConn(conn);

    const auto fit = authFdToRoleId_.find(key);
    if (fit == authFdToRoleId_.end()) {
        return;
    }
    const uint64_t roleId = fit->second;
    authFdToRoleId_.erase(fit);

    const auto sit = roleStates_.find(roleId);
    if (sit == roleStates_.end()) {
        return;
    }
    const uint64_t roomId = sit->second.roomId;
    if (sit->second.conn && sit->second.conn.get() == conn.get()) {
        g_BattleRoomManager.onPlayerKcpDisconnected(roleId, roomId, conn);
        sit->second.conn.reset();
    }
}

void BattleRoleManager::cleanupUnauthedConnections(std::time_t now, uint32_t authTimeoutSec) {
    if (authTimeoutSec == 0) {
        return;
    }
    const std::time_t deadline = static_cast<std::time_t>(authTimeoutSec);
    PendingAuthNode *node = pendingAuthLink_.getHead();
    while (node && node->time + deadline <= now) {
        auto conn = node->data;
        removeUnauthedConn(conn);
        if (conn) {
            conn->close();
            LOG("BattleRoleManager::cleanupUnauthedConnections -- close unauth fd:%d timeout:%us\n",
                conn->getfd(), (unsigned)authTimeoutSec);
        }
        node = pendingAuthLink_.getHead();
    }
}

bool BattleRoleManager::onKcpAuth(const std::shared_ptr<pb::BattleKcpAuth> &auth,
                                  const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!auth || !conn) {
        return false;
    }
    if (isSameRoomSameConn(auth->role_id(), auth->room_id(), conn)) {
        return true;
    }
    uint64_t oldRoomId = 0;
    if (getRoleRoom(auth->role_id(), &oldRoomId) && oldRoomId != auth->room_id()) {
        g_BattleRoomManager.removePlayer(oldRoomId, auth->role_id());
        onRoleRemoved(auth->role_id());
    }
    const bool ok = g_BattleRoomManager.authPlayerInRoom(auth->room_id(), auth->role_id(), auth->session_token(), conn);
    if (ok) {
        onAuthSucceeded(auth->role_id(), auth->room_id(), conn);
    }
    return ok;
}

bool BattleRoleManager::onKcpLeaveRoom(const std::shared_ptr<pb::BattleKcpLeaveRoom> &req) {
    if (!req) {
        return false;
    }
    const bool ok = g_BattleRoomManager.leavePlayerByClient(req->room_id(), req->role_id(), req->session_token());
    if (ok) {
        onRoleRemoved(req->role_id());
    }
    return ok;
}

bool BattleRoleManager::onKcpInputUpload(const std::shared_ptr<pb::BattleKcpInputUpload> &msg) {
    if (!msg || msg->room_id() == 0 || msg->role_id() == 0) {
        return false;
    }
    uint64_t boundRoomId = 0;
    if (!getRoleRoom(msg->role_id(), &boundRoomId) || boundRoomId != msg->room_id()) {
        return false;
    }
    return g_BattleRoomManager.submitInputUpload(msg->room_id(), msg->role_id(), *msg);
}

void BattleRoleManager::onAuthSucceeded(uint64_t roleId, uint64_t roomId,
                                        const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn) {
        return;
    }
    auto prev = roleStates_.find(roleId);
    if (prev != roleStates_.end() && prev->second.conn) {
        authFdToRoleId_.erase(connKey(prev->second.conn));
    }
    removeUnauthedConn(conn);
    RoleBattleState st;
    st.roomId = roomId;
    st.conn = conn;
    roleStates_[roleId] = std::move(st);
    authFdToRoleId_[connKey(conn)] = roleId;
}

void BattleRoleManager::onRoleRemoved(uint64_t roleId) {
    auto it = roleStates_.find(roleId);
    if (it != roleStates_.end() && it->second.conn) {
        authFdToRoleId_.erase(connKey(it->second.conn));
    }
    roleStates_.erase(roleId);
}

bool BattleRoleManager::isSameRoomSameConn(uint64_t roleId, uint64_t roomId,
                                           const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) const {
    if (!conn) {
        return false;
    }
    auto it = roleStates_.find(roleId);
    if (it == roleStates_.end()) {
        return false;
    }
    return it->second.roomId == roomId && it->second.conn && it->second.conn.get() == conn.get();
}

bool BattleRoleManager::getRoleRoom(uint64_t roleId, uint64_t *roomId) const {
    auto it = roleStates_.find(roleId);
    if (it == roleStates_.end()) {
        return false;
    }
    if (roomId) {
        *roomId = it->second.roomId;
    }
    return it->second.roomId != 0;
}

}

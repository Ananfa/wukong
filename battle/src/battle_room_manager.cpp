/*
 * Created by Xianke Liu on 2025/12/26.
 */

#include "battle_room_manager.h"
#include "battle_room.h"
#include "battle_config.h"
#include "battle_room_types_table.h"
#include "battle_role_manager.h"

#include "agent_manager.h"
#include "lobby_agent.h"
#include "share/const.h"

#include "corpc_routine_env.h"
#include "corpc_utils.h"

#include "lobby_service.pb.h"

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstdint>
#include <time.h>
#include <unistd.h>
#include <vector>

namespace wukong {

namespace {

int64_t timespecToNs(const struct timespec &t) {
    return static_cast<int64_t>(t.tv_sec) * 1000000000LL + static_cast<int64_t>(t.tv_nsec);
}

struct timespec nsToTimespec(int64_t ns) {
    struct timespec out;
    if (ns < 0) {
        ns = 0;
    }
    out.tv_sec = static_cast<time_t>(ns / 1000000000LL);
    out.tv_nsec = static_cast<long>(ns % 1000000000LL);
    return out;
}

int timespecCmp(const struct timespec &a, const struct timespec &b) {
    const int64_t da = timespecToNs(a);
    const int64_t db = timespecToNs(b);
    if (da < db) {
        return -1;
    }
    if (da > db) {
        return 1;
    }
    return 0;
}

struct timespec timespecAddNs(struct timespec t, int64_t ns) {
    return nsToTimespec(timespecToNs(t) + ns);
}

struct timespec timespecSub(const struct timespec &a, const struct timespec &b) {
    const int64_t d = timespecToNs(a) - timespecToNs(b);
    return nsToTimespec(d > 0 ? d : 0);
}

} // namespace

void BattleRoomManager::init() {
    inited_ = true;
    std::srand((unsigned)std::time(nullptr));

    RoutineEnvironment::startCoroutine(idleTickRoutine, nullptr);
    RoutineEnvironment::startCoroutine(frameSyncTickRoutine, nullptr);
}

std::string BattleRoomManager::makeToken(uint64_t roleId, uint64_t roomId) {
    char buf[96];
    std::snprintf(buf, sizeof(buf), "%llx-%llx-%08x", (unsigned long long)roleId, (unsigned long long)roomId, (unsigned)std::rand());
    return std::string(buf);
}

std::shared_ptr<BattleRoom> BattleRoomManager::findMatchableRoom(pb::BattleRoomMode mode, uint32_t battleDefId) {
    for (auto &kv : rooms_) {
        const std::shared_ptr<BattleRoom> &r = kv.second;
        if (!r->matchesTemplate(mode, battleDefId)) {
            continue;
        }
        if (!r->canAddPlayer()) {
            continue;
        }
        return r;
    }
    return nullptr;
}

bool BattleRoomManager::assignFromRequest(const pb::RequestBattleAssignmentRequest &req, pb::RequestBattleAssignmentResponse &rsp) {
    if (!inited_) {
        return false;
    }
    if (!req.has_player()) {
        ERROR_LOG("BattleRoomManager::assignFromRequest -- missing player\n");
        return false;
    }
    if (req.player().role_id() == 0) {
        ERROR_LOG("BattleRoomManager::assignFromRequest -- invalid role_id\n");
        return false;
    }
    const uint32_t maxP = g_BattleRoomTypesTable.getMaxPlayers(req.battle_def_id());
    if (maxP == 0) {
        ERROR_LOG("BattleRoomManager::assignFromRequest -- battle_def_id %u not in BattleRoomTypes design table\n",
                  (unsigned)req.battle_def_id());
        return false;
    }

    const ServerId lobbySid = static_cast<ServerId>(req.lobby_server_id());
    const std::time_t now = std::time(nullptr);

    std::shared_ptr<BattleRoom> room = findMatchableRoom(req.mode(), req.battle_def_id());
    if (room) {
        const std::string token = makeToken(req.player().role_id(), room->roomId());
        if (!room->addJoiningPlayer(req.player(), token, lobbySid, now)) {
            ERROR_LOG("BattleRoomManager::assignFromRequest -- add to existing room failed\n");
            return false;
        }
        rsp.set_room_id(room->roomId());
        rsp.set_kcp_host(g_BattleConfig.getExternalIp());
        rsp.set_kcp_port(g_BattleConfig.getMsgPort());
        rsp.mutable_access()->set_role_id(req.player().role_id());
        rsp.mutable_access()->set_session_token(token);
        return true;
    }

    const uint64_t roomId = nextRoomId_++;
    const std::string token = makeToken(req.player().role_id(), roomId);

    BattleRoom::PlayerSlot s;
    s.roleId = req.player().role_id();
    s.combatPayload = req.player().combat_payload();
    s.sessionToken = token;
    s.lobbyServerId = lobbySid;
    s.hasAuthed = false;
    s.assignedAt_ = now;

    std::vector<BattleRoom::PlayerSlot> slots;
    slots.push_back(std::move(s));
    room = std::make_shared<BattleRoom>(roomId, req.mode(), req.battle_def_id(), maxP, std::move(slots));
    rooms_[roomId] = room;

    rsp.set_room_id(roomId);
    rsp.set_kcp_host(g_BattleConfig.getExternalIp());
    rsp.set_kcp_port(g_BattleConfig.getMsgPort());
    rsp.mutable_access()->set_role_id(req.player().role_id());
    rsp.mutable_access()->set_session_token(token);
    return true;
}

bool BattleRoomManager::authPlayerInRoom(uint64_t roomId, uint64_t roleId, const std::string &sessionToken,
                                         const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        WARN_LOG("BattleRoomManager::authPlayerInRoom -- unknown room %llu\n", (unsigned long long)roomId);
        return false;
    }
    bool ok = it->second->tryAuth(roleId, sessionToken, conn);
    if (ok) {
        ServerId lobbySid = it->second->getPlayerLobbyServerId(roleId);
        notifyLobbyPlayerEnterBattle(lobbySid, roleId, it->second);
    }
    return ok;
}

bool BattleRoomManager::leavePlayerByClient(uint64_t roomId, uint64_t roleId, const std::string &sessionToken) {
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        return false;
    }
    ServerId lobbySid = it->second->getPlayerLobbyServerId(roleId);
    bool ok = it->second->leaveBattle(roleId, sessionToken);
    if (ok) {
        notifyLobbyPlayerLeaveBattle(lobbySid, roleId);
    }
    return ok;
}

bool BattleRoomManager::removePlayer(uint64_t roomId, uint64_t roleId) {
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        WARN_LOG("BattleRoomManager::removePlayer -- room not found %llu\n", (unsigned long long)roomId);
        return false;
    }
    if (!it->second->forceRemovePlayer(roleId)) {
        WARN_LOG("BattleRoomManager::removePlayer -- role not found role:%llu room:%llu\n",
                 (unsigned long long)roleId, (unsigned long long)roomId);
        return false;
    }
    g_BattleRoleManager.onRoleRemoved(roleId);
    return true;
}

void BattleRoomManager::onPlayerKcpDisconnected(uint64_t roleId, uint64_t roomId,
                                                   const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn || roomId == 0 || roleId == 0) {
        return;
    }
    auto it = rooms_.find(roomId);
    if (it == rooms_.end()) {
        WARN_LOG("BattleRoomManager::onPlayerKcpDisconnected -- room not found %llu role:%llu\n",
                 (unsigned long long)roomId, (unsigned long long)roleId);
        return;
    }
    it->second->detachConnectionForRole(roleId, conn);
}

void BattleRoomManager::tickAllRoomsFrameSync() {
    if (!inited_) {
        return;
    }
    for (auto &kv : rooms_) {
        kv.second->tickFrameSync();
    }
}

void *BattleRoomManager::frameSyncTickRoutine(void *arg) {
    (void)arg;
    uint32_t r = g_BattleConfig.getSyncFrameRate();
    if (r == 0) {
        r = 20;
    }
    int64_t intervalNs = 1000000000LL / static_cast<int64_t>(r);
    if (intervalNs < 10000000LL) {
        intervalNs = 10000000LL;
    }

    struct timespec deadline {};
    struct timespec now {};
    clock_gettime(CLOCK_MONOTONIC, &deadline);
    while (true) {
        deadline = timespecAddNs(deadline, intervalNs);
        clock_gettime(CLOCK_MONOTONIC, &now);

        do {
            int64_t sleepNs = timespecToNs(timespecSub(deadline, now));
            if (sleepNs <= 0) {
                break;
            }
            uint64_t usec64 = static_cast<uint64_t>(sleepNs / 1000LL);
            if (usec64 == 0) {
                usec64 = 1;
            }
            usleep(static_cast<useconds_t>(usec64));
        } while (false);

        g_BattleRoomManager.tickAllRoomsFrameSync();
    }
    return nullptr;
}

void BattleRoomManager::tickIdleRooms() {
    if (!inited_) {
        return;
    }
    const std::time_t now = std::time(nullptr);
    const uint32_t verifySec = g_BattleConfig.getVerifyTimeout();
    const uint32_t discSec = g_BattleConfig.getDisconnectTimeout();
    const uint32_t emptySec = g_BattleConfig.getRoomEmptyDestroySec();

    for (auto &kv : rooms_) {
        std::vector<std::pair<uint64_t, ServerId>> unauthRemoved;
        std::vector<std::pair<uint64_t, ServerId>> offlineRemoved;
        kv.second->expireUnauthedPlayers(now, verifySec, &unauthRemoved);
        for (const auto &pr : unauthRemoved) {
            g_BattleRoleManager.onRoleRemoved(pr.first);
            notifyLobbyPlayerLeaveBattle(pr.second, pr.first);
        }
        kv.second->expireOfflinePlayers(now, discSec, &offlineRemoved);
        for (const auto &pr : offlineRemoved) {
            g_BattleRoleManager.onRoleRemoved(pr.first);
            notifyLobbyPlayerLeaveBattle(pr.second, pr.first);
        }
    }

    std::vector<uint64_t> toRemove;
    for (const auto &kv : rooms_) {
        if (kv.second->shouldDestroy(now, emptySec)) {
            toRemove.push_back(kv.first);
        }
    }
    for (uint64_t id : toRemove) {
        auto it = rooms_.find(id);
        if (it == rooms_.end()) {
            continue;
        }
        notifyLobbyPlayersRoomDestroyed(it->second);
        rooms_.erase(it);
        LOG("BattleRoomManager::tickIdleRooms -- destroy empty room %llu (idle >= %us)\n",
            (unsigned long long)id, (unsigned)emptySec);
    }
}

void BattleRoomManager::notifyLobbyPlayerEnterBattle(ServerId lobbySid, uint64_t roleId, const std::shared_ptr<BattleRoom> &room) {
    if (lobbySid == 0 || !room) {
        return;
    }
    auto *agent = static_cast<LobbyAgent *>(g_AgentManager.getAgent(SERVER_TYPE_LOBBY));
    if (!agent) {
        return;
    }
    pb::PlayerBattleStateNotify n;
    n.set_role_id(roleId);
    n.set_in_battle(true);
    n.set_battle_server_id(static_cast<int32_t>(g_BattleConfig.getId()));
    n.set_room_id(room->roomId());
    n.set_kcp_host(g_BattleConfig.getExternalIp());
    n.set_kcp_port(g_BattleConfig.getMsgPort());
    n.set_battle_def_id(room->battleDefId());
    agent->notifyPlayerBattleState(lobbySid, n);
}

void BattleRoomManager::notifyLobbyPlayerLeaveBattle(ServerId lobbySid, uint64_t roleId) {
    if (lobbySid == 0) {
        return;
    }
    auto *agent = static_cast<LobbyAgent *>(g_AgentManager.getAgent(SERVER_TYPE_LOBBY));
    if (!agent) {
        return;
    }
    pb::PlayerBattleStateNotify n;
    n.set_role_id(roleId);
    n.set_in_battle(false);
    agent->notifyPlayerBattleState(lobbySid, n);
}

void BattleRoomManager::notifyLobbyPlayersRoomDestroyed(const std::shared_ptr<BattleRoom> &room) {
    if (!room) {
        return;
    }
    room->forEachPlayer([this](const BattleRoom::PlayerSlot &p) {
        g_BattleRoleManager.onRoleRemoved(p.roleId);
        if (p.lobbyServerId == 0) {
            return;
        }
        notifyLobbyPlayerLeaveBattle(p.lobbyServerId, p.roleId);
    });
}

void *BattleRoomManager::idleTickRoutine(void *arg) {
    (void)arg;
    while (true) {
        sleep(1);
        g_BattleRoomManager.tickIdleRooms();
    }
    return nullptr;
}

}

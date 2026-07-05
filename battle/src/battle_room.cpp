/*
 * Created by Xianke Liu on 2025/12/26.
 */

#include "battle_room.h"
#include "battle_config.h"
#include "battle_const.h"
#include "battle_sync.pb.h"

#include "corpc_utils.h"

namespace wukong {

BattleRoom::BattleRoom(uint64_t roomId, pb::BattleRoomMode mode, uint32_t battleDefId, uint32_t maxPlayers,
                       std::vector<PlayerSlot> &&players)
    : roomId_(roomId), mode_(mode), battleDefId_(battleDefId), maxPlayers_(maxPlayers > 0 ? maxPlayers : 1) {
    for (auto &p : players) {
        players_[p.roleId] = std::move(p);
    }
    if (maxPlayers_ < players_.size()) {
        maxPlayers_ = static_cast<uint32_t>(players_.size());
    }
    noteNoPlayersSince();
}

ServerId BattleRoom::getPlayerLobbyServerId(uint64_t roleId) const {
    auto it = players_.find(roleId);
    if (it != players_.end()) {
        return it->second.lobbyServerId;
    }
    return 0;
}

void BattleRoom::forEachPlayer(const std::function<void(const PlayerSlot &)> &fn) const {
    for (const auto &kv : players_) {
        fn(kv.second);
    }
}

bool BattleRoom::hasAnyOnlineAuthedPlayer() const {
    for (const auto &kv : players_) {
        const auto &p = kv.second;
        if (p.hasAuthed && p.conn) {
            return true;
        }
    }
    return false;
}

void BattleRoom::noteNoPlayersSince() {
    if (players_.empty()) {
        if (noPlayersSince_ == 0) {
            noPlayersSince_ = std::time(nullptr);
        }
    } else {
        noPlayersSince_ = 0;
    }
}

void BattleRoom::sendSnapshotTo(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn) {
        return;
    }
    auto snap = std::make_shared<pb::BattleRoomSnapshot>();
    snap->set_room_id(roomId_);
    snap->set_logic_frame(syncFrameIndex_);
    snap->set_frame_rate(g_BattleConfig.getSyncFrameRate());
    snap->set_room_state(pb::BATTLE_ROOM_STATE_RUNNING);
    conn->send(BATTLE_KCP_MSG_SNAPSHOT, false, false, false, 0, snap);
}

void BattleRoom::tickFrameSync() {
    if (!hasAnyOnlineAuthedPlayer()) {
        return;
    }
    ++syncFrameIndex_;
    auto fr = std::make_shared<pb::BattleFrameSync>();
    fr->set_room_id(roomId_);
    fr->set_frame_index(syncFrameIndex_);
    for (auto &kv : players_) {
        auto &p = kv.second;
        if (p.hasAuthed && p.conn) {
            p.conn->send(BATTLE_KCP_MSG_FRAME_SYNC, false, false, false, 0, fr);
        }
    }
}

bool BattleRoom::tryAuth(uint64_t roleId, const std::string &token, const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    auto it = players_.find(roleId);
    if (it == players_.end()) {
        WARN_LOG("BattleRoom::tryAuth -- role not in room role:%llu room:%llu\n", (unsigned long long)roleId, (unsigned long long)roomId_);
        return false;
    }
    auto &p = it->second;
    if (p.sessionToken != token) {
        WARN_LOG("BattleRoom::tryAuth -- token mismatch role:%llu room:%llu\n", (unsigned long long)roleId, (unsigned long long)roomId_);
        return false;
    }
    p.conn = conn;
    p.hasAuthed = true;
    p.offlineSince_ = 0;
    p.assignedAt_ = 0;
    LOG("BattleRoom::tryAuth -- ok role:%llu room:%llu\n", (unsigned long long)roleId, (unsigned long long)roomId_);
    sendSnapshotTo(conn);
    return true;
}

void BattleRoom::detachConnectionForRole(uint64_t roleId, const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    auto it = players_.find(roleId);
    if (it == players_.end()) {
        return;
    }
    auto &p = it->second;
    if (!p.conn || p.conn.get() != conn.get()) {
        return;
    }
    p.conn.reset();
    if (p.hasAuthed) {
        p.offlineSince_ = std::time(nullptr);
    }
    LOG("BattleRoom::detachConnectionForRole role:%llu room:%llu\n",
        (unsigned long long)roleId, (unsigned long long)roomId_);
}

bool BattleRoom::leaveBattle(uint64_t roleId, const std::string &token) {
    auto it = players_.find(roleId);
    if (it != players_.end() && it->second.sessionToken == token) {
        players_.erase(it);
        LOG("BattleRoom::leaveBattle -- role:%llu left room:%llu\n", (unsigned long long)roleId, (unsigned long long)roomId_);
        noteNoPlayersSince();
        return true;
    }
    return false;
}

bool BattleRoom::forceRemovePlayer(uint64_t roleId) {
    auto it = players_.find(roleId);
    if (it != players_.end()) {
        if (it->second.conn) {
            it->second.conn->close();
        }
        players_.erase(it);
        LOG("BattleRoom::forceRemovePlayer -- role:%llu room:%llu\n", (unsigned long long)roleId, (unsigned long long)roomId_);
        noteNoPlayersSince();
        return true;
    }
    return false;
}

bool BattleRoom::addJoiningPlayer(const pb::BattlePlayerInitData &player, const std::string &token, ServerId lobbyServerId,
                                  std::time_t assignedAt) {
    if (player.role_id() == 0) {
        return false;
    }
    if (!canAddPlayer()) {
        return false;
    }
    if (players_.find(player.role_id()) != players_.end()) {
        return false;
    }
    PlayerSlot s;
    s.roleId = player.role_id();
    s.combatPayload = player.combat_payload();
    s.sessionToken = token;
    s.lobbyServerId = lobbyServerId;
    s.hasAuthed = false;
    s.assignedAt_ = assignedAt;
    players_[s.roleId] = std::move(s);
    noteNoPlayersSince();
    return true;
}

bool BattleRoom::shouldDestroy(std::time_t now, uint32_t emptySeconds) const {
    if (noPlayersSince_ == 0) {
        return false;
    }
    return now >= noPlayersSince_ + static_cast<std::time_t>(emptySeconds);
}

void BattleRoom::expireOfflinePlayers(std::time_t now, uint32_t offlineKickSec, std::vector<std::pair<uint64_t, ServerId>> *removed) {
    if (offlineKickSec == 0) {
        return;
    }
    const std::time_t deadline = static_cast<std::time_t>(offlineKickSec);
    for (auto it = players_.begin(); it != players_.end();) {
        auto &p = it->second;
        if (p.hasAuthed && !p.conn && p.offlineSince_ != 0 && now >= p.offlineSince_ + deadline) {
            if (removed) {
                removed->push_back(std::make_pair(p.roleId, p.lobbyServerId));
            }
            it = players_.erase(it);
            continue;
        }
        ++it;
    }
    noteNoPlayersSince();
}

void BattleRoom::expireUnauthedPlayers(std::time_t now, uint32_t verifySec, std::vector<std::pair<uint64_t, ServerId>> *removed) {
    if (verifySec == 0) {
        return;
    }
    const std::time_t deadline = static_cast<std::time_t>(verifySec);
    for (auto it = players_.begin(); it != players_.end();) {
        auto &p = it->second;
        if (!p.hasAuthed && p.assignedAt_ != 0 && now >= p.assignedAt_ + deadline) {
            if (removed) {
                removed->push_back(std::make_pair(p.roleId, p.lobbyServerId));
            }
            it = players_.erase(it);
            continue;
        }
        ++it;
    }
    noteNoPlayersSince();
}

}

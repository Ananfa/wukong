/*
 * Created by Xianke Liu on 2025/12/26.
 */

#include "battle_room.h"
#include "battle_config.h"
#include "battle_const.h"
#include "battle_sync.pb.h"

#include "GameCore.h"
#include "Types.h"
#include "Constants.h"

#include "corpc_utils.h"

#include <algorithm>
#include <cstdio>

namespace wukong {

namespace {

TankBattle::PlayerInput buildPlayerInput(uint32_t playerId, uint32_t frame, const pb::BattleKcpInputUpload &msg) {
    TankBattle::PlayerInput input;
    input.playerId = playerId;
    input.frame = frame;
    input.moveX = static_cast<int16_t>(msg.move_x());
    input.moveY = static_cast<int16_t>(msg.move_y());
    input.aimX = static_cast<int16_t>(msg.aim_x());
    input.aimY = static_cast<int16_t>(msg.aim_y());
    input.fire = msg.fire();
    input.useAbility = msg.use_ability();
    input.timestamp = msg.timestamp();
    return input;
}

} // namespace

BattleRoom::BattleRoom(uint64_t roomId, pb::BattleRoomMode mode, uint32_t battleDefId, uint32_t maxPlayers,
                       std::vector<PlayerSlot> &&players)
    : roomId_(roomId), mode_(mode), battleDefId_(battleDefId), maxPlayers_(maxPlayers > 0 ? maxPlayers : 1) {
    randomSeed_ = static_cast<uint32_t>(roomId_ ^ (roomId_ >> 32));
    if (randomSeed_ == 0) {
        randomSeed_ = 1;
    }
    for (auto &p : players) {
        players_[p.roleId] = std::move(p);
    }
    if (maxPlayers_ < players_.size()) {
        maxPlayers_ = static_cast<uint32_t>(players_.size());
    }
    noteNoPlayersSince();
}

BattleRoom::~BattleRoom() = default;

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

void BattleRoom::ensureSimulationInitialized() {
    if (gameCore_) {
        return;
    }
    gameCore_.reset(new TankBattle::GameCore());
    if (!gameCore_->Initialize(maxPlayers_)) {
        gameCore_.reset();
        ERROR_LOG("BattleRoom::ensureSimulationInitialized -- GameCore init failed room:%llu\n",
                  (unsigned long long)roomId_);
        return;
    }
    gameCore_->SetRandomSeed(randomSeed_);
}

TankBattle::Faction BattleRoom::resolveFactionForRole(uint64_t roleId) const {
    static const TankBattle::Faction kOrder[] = {
        TankBattle::Faction::Soviet,
        TankBattle::Faction::USA,
        TankBattle::Faction::Germany,
        TankBattle::Faction::Italy
    };
    return kOrder[roleId % 4];
}

bool BattleRoom::addPlayerToSimulation(uint64_t roleId) {
    ensureSimulationInitialized();
    if (!gameCore_) {
        return false;
    }

    auto it = players_.find(roleId);
    if (it == players_.end()) {
        return false;
    }
    PlayerSlot &slot = it->second;
    if (slot.gamePlayerId != 0) {
        return true;
    }

    char nameBuf[64];
    std::snprintf(nameBuf, sizeof(nameBuf), "role_%llu", (unsigned long long)roleId);
    const TankBattle::Faction faction = resolveFactionForRole(roleId);
    const uint32_t playerId = gameCore_->AddPlayer(nameBuf, faction);
    if (playerId == 0) {
        WARN_LOG("BattleRoom::addPlayerToSimulation -- AddPlayer failed role:%llu room:%llu\n",
                 (unsigned long long)roleId, (unsigned long long)roomId_);
        return false;
    }
    slot.gamePlayerId = playerId;
    slot.gameFaction = static_cast<int>(faction);

    if (!simulationStarted_) {
        gameCore_->StartGame();
        simulationStarted_ = true;
        syncFrameIndex_ = gameCore_->GetFrame();
        LOG("BattleRoom::addPlayerToSimulation -- StartGame room:%llu seed:%u frame:%u\n",
            (unsigned long long)roomId_, (unsigned)randomSeed_, (unsigned)syncFrameIndex_);
    }
    return true;
}

void BattleRoom::removePlayerFromSimulation(uint64_t roleId) {
    if (!gameCore_) {
        return;
    }
    auto it = players_.find(roleId);
    if (it == players_.end() || it->second.gamePlayerId == 0) {
        return;
    }
    const uint32_t playerId = it->second.gamePlayerId;
    gameCore_->RemovePlayer(playerId);
    pendingFrameInputs_.erase(playerId);
    it->second.gamePlayerId = 0;
}

void BattleRoom::sendSnapshotTo(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn) {
        return;
    }
    auto snap = std::make_shared<pb::BattleRoomSnapshot>();
    snap->set_room_id(roomId_);
    snap->set_logic_frame(simulationStarted_ && gameCore_ ? gameCore_->GetFrame() : syncFrameIndex_);
    snap->set_frame_rate(g_BattleConfig.getSyncFrameRate());
    snap->set_random_seed(randomSeed_);
    if (simulationStarted_ && gameCore_ && gameCore_->IsGameOver()) {
        snap->set_room_state(pb::BATTLE_ROOM_STATE_ENDED);
    } else if (simulationStarted_) {
        snap->set_room_state(pb::BATTLE_ROOM_STATE_RUNNING);
    } else {
        snap->set_room_state(pb::BATTLE_ROOM_STATE_WAITING);
    }
    for (const auto &kv : players_) {
        const PlayerSlot &slot = kv.second;
        if (slot.gamePlayerId == 0) {
            continue;
        }
        pb::BattleSnapshotPlayer *p = snap->add_players();
        p->set_role_id(slot.roleId);
        p->set_player_id(slot.gamePlayerId);
        p->set_faction(slot.gameFaction);
    }
    conn->send(BATTLE_KCP_MSG_SNAPSHOT, false, false, false, 0, snap);
}

void BattleRoom::fillProtoInput(const TankBattle::PlayerInput &src, pb::BattlePlayerFrameInput *dst, uint64_t roleId) {
    if (!dst) {
        return;
    }
    dst->set_player_id(src.playerId);
    dst->set_role_id(roleId);
    dst->set_frame(src.frame);
    dst->set_move_x(src.moveX);
    dst->set_move_y(src.moveY);
    dst->set_aim_x(src.aimX);
    dst->set_aim_y(src.aimY);
    dst->set_fire(src.fire);
    dst->set_use_ability(src.useAbility);
    dst->set_timestamp(src.timestamp);
}

void BattleRoom::submitInputUpload(uint64_t roleId, const pb::BattleKcpInputUpload &msg) {
    if (!simulationStarted_ || !gameCore_ || gameCore_->IsGameOver()) {
        return;
    }

    auto it = players_.find(roleId);
    if (it == players_.end() || !it->second.hasAuthed) {
        return;
    }
    PlayerSlot &slot = it->second;
    if (slot.gamePlayerId == 0) {
        if (!addPlayerToSimulation(roleId)) {
            return;
        }
    }

    const uint32_t expectedFrame = gameCore_->GetFrame() + 1;
    const uint32_t frame = msg.frame() != 0 ? msg.frame() : expectedFrame;
    if (frame != expectedFrame) {
        return;
    }

    pendingFrameInputs_[slot.gamePlayerId] = buildPlayerInput(slot.gamePlayerId, frame, msg);
}

void BattleRoom::tickFrameSync() {
    if (!hasAnyOnlineAuthedPlayer()) {
        return;
    }
    if (!simulationStarted_ || !gameCore_) {
        return;
    }
    if (gameCore_->IsGameOver()) {
        return;
    }

    const uint32_t nextFrame = gameCore_->GetFrame() + 1;
    std::vector<TankBattle::PlayerInput> frameInputs;
    frameInputs.reserve(pendingFrameInputs_.size());
    for (const auto &entry : pendingFrameInputs_) {
        frameInputs.push_back(entry.second);
    }

    if (!gameCore_->SetFrameInputs(nextFrame, frameInputs.empty() ? nullptr : frameInputs.data(), frameInputs.size())) {
        WARN_LOG("BattleRoom::tickFrameSync -- SetFrameInputs failed room:%llu frame:%u\n",
                 (unsigned long long)roomId_, (unsigned)nextFrame);
        return;
    }

    gameCore_->AdvanceSimulation();
    syncFrameIndex_ = gameCore_->GetFrame();
    pendingFrameInputs_.clear();

    auto fr = std::make_shared<pb::BattleFrameSync>();
    fr->set_room_id(roomId_);
    fr->set_frame_index(syncFrameIndex_);
    for (const auto &entry : players_) {
        const PlayerSlot &slot = entry.second;
        auto inputIt = std::find_if(frameInputs.begin(), frameInputs.end(),
            [&slot](const TankBattle::PlayerInput &input) {
                return input.playerId == slot.gamePlayerId;
            });
        if (inputIt != frameInputs.end()) {
            fillProtoInput(*inputIt, fr->add_inputs(), slot.roleId);
        }
    }

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

    addPlayerToSimulation(roleId);
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
        removePlayerFromSimulation(roleId);
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
        removePlayerFromSimulation(roleId);
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
            removePlayerFromSimulation(p.roleId);
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

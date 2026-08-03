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

BattleRoom::BattleRoom(uint64_t roomId, pb::BattleRoomMode mode, uint32_t battleDefId,
                       const BattleRoomTypeDef &typeDef, std::vector<PlayerSlot> &&players)
    : roomId_(roomId)
    , mode_(mode)
    , battleDefId_(battleDefId)
    , maxPlayers_(typeDef.maxPlayers > 0 ? typeDef.maxPlayers : 1)
    , slotsPerFaction_(typeDef.slotsPerFaction > 0 ? typeDef.slotsPerFaction : 2)
    , battleDurationSec_(typeDef.battleDurationSec)
    , joinWindowSec_(typeDef.joinWindowSec) {
    for (int i = 0; i < kFactionCount; ++i) {
        humanCount_[i] = 0;
    }
    randomSeed_ = static_cast<uint32_t>(roomId_ ^ (roomId_ >> 32));
    if (randomSeed_ == 0) {
        randomSeed_ = 1;
    }
    for (auto &p : players) {
        if (p.reservedFaction < 0 || p.reservedFaction >= kFactionCount) {
            p.reservedFaction = normalizeFaction(p.preferredFaction, p.roleId);
            p.preferredFaction = p.reservedFaction;
        }
        ++humanCount_[p.reservedFaction];
        players_[p.roleId] = std::move(p);
    }
    if (maxPlayers_ < players_.size()) {
        maxPlayers_ = static_cast<uint32_t>(players_.size());
    }
    noteNoPlayersSince();
}

BattleRoom::~BattleRoom() = default;

int BattleRoom::normalizeFaction(int faction, uint64_t roleId) {
    if (faction >= 0 && faction < kFactionCount) {
        return faction;
    }
    return static_cast<int>(roleId % static_cast<uint64_t>(kFactionCount));
}

int BattleRoom::humanCount(int faction) const {
    if (faction < 0 || faction >= kFactionCount) {
        return 0;
    }
    return humanCount_[faction];
}

bool BattleRoom::isJoinWindowOpen(std::time_t now) const {
    if (!simulationStarted_ || battleStartedAt_ == 0) {
        return true;
    }
    return now < battleStartedAt_ + static_cast<std::time_t>(joinWindowSec_);
}

bool BattleRoom::isBattleTimeUp(std::time_t now) const {
    if (battleDurationSec_ == 0 || !simulationStarted_ || battleStartedAt_ == 0) {
        return false;
    }
    return now >= battleStartedAt_ + static_cast<std::time_t>(battleDurationSec_);
}

bool BattleRoom::canJoinFaction(int faction, std::time_t now) const {
    if (faction < 0 || faction >= kFactionCount) {
        return false;
    }
    if (players_.size() >= maxPlayers_) {
        return false;
    }
    if (humanCount_[faction] >= static_cast<int>(slotsPerFaction_)) {
        return false;
    }
    if (isBattleTimeUp(now)) {
        return false;
    }
    if (!isJoinWindowOpen(now)) {
        return false;
    }
    if (gameCore_ && gameCore_->IsGameOver()) {
        return false;
    }
    return true;
}

void BattleRoom::releaseFactionSeat(int faction) {
    if (faction < 0 || faction >= kFactionCount) {
        return;
    }
    if (humanCount_[faction] > 0) {
        --humanCount_[faction];
    }
}

std::unordered_map<uint64_t, BattleRoom::PlayerSlot>::iterator BattleRoom::erasePlayerSlot(
    std::unordered_map<uint64_t, PlayerSlot>::iterator it) {
    releaseFactionSeat(it->second.reservedFaction);
    removePlayerFromSimulation(it->second.roleId);
    std::unordered_map<uint64_t, PlayerSlot>::iterator next = players_.erase(it);
    noteNoPlayersSince();
    return next;
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

    // 与 Unity Assets/Config/MapObstacles.json 同一份策划配置，保证寻路/碰撞一致
    const std::string &mapJson = g_BattleConfig.getMapObstaclesJson();
    if (!mapJson.empty()) {
        if (!gameCore_->LoadMapObstaclesFromJson(mapJson)) {
            ERROR_LOG("BattleRoom::ensureSimulationInitialized -- LoadMapObstaclesFromJson failed room:%llu path:%s\n",
                      (unsigned long long)roomId_, g_BattleConfig.getMapObstaclesPath().c_str());
        } else {
            LOG("BattleRoom::ensureSimulationInitialized -- map loaded room:%llu from %s\n",
                (unsigned long long)roomId_, g_BattleConfig.getMapObstaclesPath().c_str());
        }
    } else {
        WARN_LOG("BattleRoom::ensureSimulationInitialized -- no map JSON; using GameCore built-in defaults room:%llu\n",
                 (unsigned long long)roomId_);
    }

    gameCore_->SetRandomSeed(randomSeed_);
    gameCore_->SetSlotsPerFaction(slotsPerFaction_);
}

TankBattle::Faction BattleRoom::resolveFactionForRole(uint64_t roleId) const {
    auto it = players_.find(roleId);
    if (it != players_.end()) {
        const int f = it->second.reservedFaction >= 0 ? it->second.reservedFaction : it->second.preferredFaction;
        return static_cast<TankBattle::Faction>(normalizeFaction(f, roleId));
    }
    return static_cast<TankBattle::Faction>(normalizeFaction(-1, roleId));
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

    if (!simulationStarted_) {
        gameCore_->StartGameAIOnly();
        simulationStarted_ = true;
        battleStartedAt_ = std::time(nullptr);
        syncFrameIndex_ = gameCore_->GetFrame();
        LOG("BattleRoom::addPlayerToSimulation -- StartGameAIOnly room:%llu seed:%u slots:%u\n",
            (unsigned long long)roomId_, (unsigned)randomSeed_, (unsigned)slotsPerFaction_);
    }

    char nameBuf[64];
    std::snprintf(nameBuf, sizeof(nameBuf), "role_%llu", (unsigned long long)roleId);
    const TankBattle::Faction faction = resolveFactionForRole(roleId);
    uint32_t tankId = 0;
    const uint32_t playerId = gameCore_->PossessAITank(nameBuf, faction, &tankId);
    if (playerId == 0) {
        WARN_LOG("BattleRoom::addPlayerToSimulation -- PossessAITank failed role:%llu room:%llu faction:%d\n",
                 (unsigned long long)roleId, (unsigned long long)roomId_, static_cast<int>(faction));
        return false;
    }
    slot.gamePlayerId = playerId;
    slot.gameTankId = tankId;
    slot.gameFaction = static_cast<int>(faction);

    PendingControlEvent ev;
    ev.type = pb::BATTLE_CONTROL_POSSESS;
    ev.roleId = roleId;
    ev.playerId = playerId;
    ev.tankId = tankId;
    ev.faction = slot.gameFaction;
    ev.name = nameBuf;
    queueControlEvent(ev);

    LOG("BattleRoom::addPlayerToSimulation -- possessed role:%llu playerId:%u tankId:%u faction:%d room:%llu\n",
        (unsigned long long)roleId, (unsigned)playerId, (unsigned)tankId, slot.gameFaction,
        (unsigned long long)roomId_);
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
    const uint32_t tankId = it->second.gameTankId;
    const int faction = it->second.gameFaction;

    PendingControlEvent ev;
    ev.type = pb::BATTLE_CONTROL_RELEASE;
    ev.roleId = roleId;
    ev.playerId = playerId;
    ev.tankId = tankId;
    ev.faction = faction;
    queueControlEvent(ev);

    gameCore_->ReleaseToAI(playerId);
    pendingFrameInputs_.erase(playerId);
    it->second.gamePlayerId = 0;
    it->second.gameTankId = 0;
}

void BattleRoom::queueControlEvent(const PendingControlEvent &ev) {
    pendingControlEvents_.push_back(ev);
}

std::shared_ptr<pb::BattleRoomSnapshot> BattleRoom::buildSnapshotProto() {
    auto snap = std::make_shared<pb::BattleRoomSnapshot>();
    snap->set_room_id(roomId_);
    snap->set_frame_rate(g_BattleConfig.getSyncFrameRate());
    snap->set_random_seed(randomSeed_);
    snap->set_slots_per_faction(slotsPerFaction_);

    const std::time_t now = std::time(nullptr);
    if (isBattleTimeUp(now) || (simulationStarted_ && gameCore_ && gameCore_->IsGameOver())) {
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
        p->set_tank_id(slot.gameTankId);
    }

    if (!gameCore_) {
        snap->set_logic_frame(syncFrameIndex_);
        return snap;
    }

    // 导出含 AI 记忆；中途加入靠 ai_memories 对齐，勿 ClearAiMemory。
    const TankBattle::GameLogicSnapshot logic = gameCore_->ExportLogicSnapshot();
    snap->set_logic_frame(logic.frame);
    snap->set_next_player_id(logic.nextPlayerId);
    snap->set_next_tank_id(logic.nextTankId);
    snap->set_next_bullet_id(logic.nextBulletId);
    snap->set_game_state(logic.gameState);
    snap->set_random_seed(logic.randomSeed);
    snap->set_slots_per_faction(logic.slotsPerFaction);
    for (int i = 0; i < 4; ++i) {
        snap->add_faction_kills(logic.factionKills[i]);
        snap->add_faction_deaths(logic.factionDeaths[i]);
    }
    for (size_t i = 0; i < logic.tanks.size(); ++i) {
        const TankBattle::TankLogicSnapshot &t = logic.tanks[i];
        pb::BattleSnapshotTank *out = snap->add_tanks();
        out->set_id(t.id);
        out->set_player_id(t.playerId);
        out->set_faction(t.faction);
        out->set_type(t.type);
        out->set_pos_x(t.posX);
        out->set_pos_y(t.posY);
        out->set_vel_x(t.velX);
        out->set_vel_y(t.velY);
        out->set_rotation(t.rotation);
        out->set_turret_rotation(t.turretRotation);
        out->set_hp(t.hp);
        out->set_max_hp(t.maxHp);
        out->set_shield_frames(t.shieldFrames);
        out->set_speed_boost_frames(t.speedBoostFrames);
        out->set_rapid_fire_frames(t.rapidFireFrames);
        out->set_ability_cooldown_frames(t.abilityCooldownFrames);
        out->set_reload_frames(t.reloadFrames);
        out->set_reload_duration_frames(t.reloadDurationFrames);
        out->set_recoil_vel_x(t.recoilVelX);
        out->set_recoil_vel_y(t.recoilVelY);
        out->set_locked_target_id(t.lockedTargetId);
        out->set_ai_move_mode(t.aiMoveMode);
        out->set_respawn_frames(t.respawnFrames);
        out->set_spawn_protection_frames(t.spawnProtectionFrames);
        out->set_charged_shot(t.chargedShot);
        out->set_is_player(t.isPlayer);
        out->set_is_alive(t.isAlive || t.hp > 0);
    }
    for (size_t i = 0; i < logic.aiMemories.size(); ++i) {
        const TankBattle::AiTankMemorySnapshot &m = logic.aiMemories[i];
        pb::BattleSnapshotAiMemory *out = snap->add_ai_memories();
        out->set_tank_id(m.tankId);
        out->set_wander_heading(m.wanderHeading);
        out->set_strafe_sign(m.strafeSign);
        out->set_strafe_switch_frames(m.strafeSwitchFrames);
        out->set_wander_goal_serial(m.wanderGoalSerial);
        out->set_path_goal_x(m.pathGoalX);
        out->set_path_goal_y(m.pathGoalY);
        out->set_path_target_id(m.pathTargetId);
        out->set_path_move_mode(m.pathMoveMode);
        out->set_path_recalc_frames(m.pathRecalcFrames);
        out->set_wander_path_goal_x(m.wanderPathGoalX);
        out->set_wander_path_goal_y(m.wanderPathGoalY);
        out->set_wander_path_frames(m.wanderPathFrames);
        out->set_path_waypoint_index(m.pathWaypointIndex);
        for (size_t c = 0; c < m.pathWaypointCoords.size(); ++c) {
            out->add_path_waypoint_coords(m.pathWaypointCoords[c]);
        }
    }
    for (size_t i = 0; i < logic.bullets.size(); ++i) {
        const TankBattle::BulletLogicSnapshot &b = logic.bullets[i];
        pb::BattleSnapshotBullet *out = snap->add_bullets();
        out->set_id(b.id);
        out->set_owner_id(b.ownerId);
        out->set_pos_x(b.posX);
        out->set_pos_y(b.posY);
        out->set_vel_x(b.velX);
        out->set_vel_y(b.velY);
        out->set_damage(b.damage);
        out->set_life_frames(b.lifeFrames);
        out->set_penetrating(b.penetrating);
        for (size_t d = 0; d < b.damagedTankIds.size(); ++d) {
            out->add_damaged_tank_ids(b.damagedTankIds[d]);
        }
    }
    return snap;
}

void BattleRoom::sendSnapshotTo(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) {
    if (!conn) {
        return;
    }
    conn->send(BATTLE_KCP_MSG_SNAPSHOT, false, false, false, 0, buildSnapshotProto());
}

void BattleRoom::broadcastSnapshot() {
    auto snap = buildSnapshotProto();
    for (auto &kv : players_) {
        if (kv.second.hasAuthed && kv.second.conn) {
            kv.second.conn->send(BATTLE_KCP_MSG_SNAPSHOT, false, false, false, 0, snap);
        }
    }
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
    if (gameCore_->IsGameOver() || isBattleTimeUp(std::time(nullptr))) {
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

    // 控制事件打进本帧 sync（服端已在 Auth/Leave 时应用到 GameCore；客户端按事件幂等应用）
    auto fr = std::make_shared<pb::BattleFrameSync>();
    fr->set_room_id(roomId_);
    const bool hasControlEvents = !pendingControlEvents_.empty();
    for (size_t i = 0; i < pendingControlEvents_.size(); ++i) {
        const PendingControlEvent &ev = pendingControlEvents_[i];
        pb::BattleControlEvent *out = fr->add_control_events();
        out->set_type(ev.type);
        out->set_role_id(ev.roleId);
        out->set_player_id(ev.playerId);
        out->set_tank_id(ev.tankId);
        out->set_faction(ev.faction);
        out->set_name(ev.name);
    }
    pendingControlEvents_.clear();

    (void)hasControlEvents;
    pendingFullSnapshotRoleIds_.clear();

    gameCore_->AdvanceSimulation();
    syncFrameIndex_ = gameCore_->GetFrame();
    pendingFrameInputs_.clear();

    // 第 600 帧（30Hz ≈ 第 20 秒）打印定点 Snapshot，供与客户端对比
    if (syncFrameIndex_ == 600) {
        const std::string dump = gameCore_->FormatCompareSnapshot("SERVER");
        LOG("%s", dump.c_str());
    }

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

    if (!addPlayerToSimulation(roleId)) {
        WARN_LOG("BattleRoom::tryAuth -- possess failed role:%llu room:%llu\n",
                 (unsigned long long)roleId, (unsigned long long)roomId_);
        p.conn.reset();
        p.hasAuthed = false;
        return false;
    }
    // Auth 后立刻发全量 Snapshot（含坦克坐标 + AI 记忆）。勿走「无 tanks 的旧快照」，
    // 否则客户端会 Bootstrap/StartGameAIOnly，坦克全在出生点，德军会杀进苏联老家。
    sendSnapshotTo(conn);
    if (gameCore_) {
        const TankBattle::GameLogicSnapshot logic = gameCore_->ExportLogicSnapshot();
        LOG("BattleRoom::tryAuth -- full snapshot sent role:%llu room:%llu frame:%u tanks:%u aiMem:%u\n",
            (unsigned long long)roleId, (unsigned long long)roomId_,
            (unsigned)logic.frame, (unsigned)logic.tanks.size(), (unsigned)logic.aiMemories.size());
    }
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
        LOG("BattleRoom::leaveBattle -- role:%llu left room:%llu\n", (unsigned long long)roleId, (unsigned long long)roomId_);
        erasePlayerSlot(it);
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
        LOG("BattleRoom::forceRemovePlayer -- role:%llu room:%llu\n", (unsigned long long)roleId, (unsigned long long)roomId_);
        erasePlayerSlot(it);
        return true;
    }
    return false;
}

bool BattleRoom::addJoiningPlayer(const pb::BattlePlayerInitData &player, const std::string &token, ServerId lobbyServerId,
                                  std::time_t assignedAt) {
    if (player.role_id() == 0) {
        return false;
    }
    if (players_.find(player.role_id()) != players_.end()) {
        return false;
    }
    const int faction = normalizeFaction(player.faction(), player.role_id());
    if (!canJoinFaction(faction, assignedAt)) {
        return false;
    }
    PlayerSlot s;
    s.roleId = player.role_id();
    s.combatPayload = player.combat_payload();
    s.preferredFaction = faction;
    s.reservedFaction = faction;
    s.sessionToken = token;
    s.lobbyServerId = lobbyServerId;
    s.hasAuthed = false;
    s.assignedAt_ = assignedAt;
    ++humanCount_[faction];
    players_[s.roleId] = std::move(s);
    noteNoPlayersSince();
    return true;
}

bool BattleRoom::rebindPlayerForReconnect(uint64_t roleId, const std::string &token, ServerId lobbyServerId,
                                          std::time_t assignedAt) {
    auto it = players_.find(roleId);
    if (it == players_.end()) {
        return false;
    }
    if (isBattleTimeUp(assignedAt) || (gameCore_ && gameCore_->IsGameOver())) {
        return false;
    }
    PlayerSlot &s = it->second;
    s.sessionToken = token;
    s.lobbyServerId = lobbyServerId;
    s.hasAuthed = false;
    s.conn.reset();
    s.offlineSince_ = 0;
    s.assignedAt_ = assignedAt;
    // 保留 gamePlayerId / gameTankId / reservedFaction —— 坦克仍由该玩家占用
    noteNoPlayersSince();
    LOG("BattleRoom::rebindPlayerForReconnect -- role:%llu room:%llu playerId:%u tankId:%u\n",
        (unsigned long long)roleId, (unsigned long long)roomId_,
        (unsigned)s.gamePlayerId, (unsigned)s.gameTankId);
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
            it = erasePlayerSlot(it);
            continue;
        }
        ++it;
    }
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
            // 未鉴权也占了 humanCount，需释放席位
            releaseFactionSeat(p.reservedFaction);
            it = players_.erase(it);
            noteNoPlayersSince();
            continue;
        }
        ++it;
    }
}

}

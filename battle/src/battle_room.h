/*
 * 单个帧同步战斗房间（单人/多人共用）
 *
 * 匹配进房：Lobby RPC 登记玩家后客户端 KCP 鉴权；鉴权成功后才计为「在战局中」推进帧同步。
 * 掉线：断开 conn，hasAuthed 仍为 true；超过 disconnectTimeout 未重连则从房间移除并通知 Lobby。
 * 未鉴权席位：超过 verifyTimeout 未 KCP 鉴权则移除并通知 Lobby（取消等待进房）。
 * 主动离开：交回 AI、释放阵营席位。
 * 房间销毁：players_ 为空时起算，持续 roomEmptyDestroySec 后销毁。
 *
 * 接管模式：开战刷满各阵营 AI 槽；玩家进房 Possess；humanCount_[faction] 在分配席位时维护，不查 GameCore。
 */
#ifndef wukong_battle_room_h
#define wukong_battle_room_h

#include "battle_service.pb.h"
#include "battle_sync.pb.h"
#include "battle_room_types_table.h"
#include "corpc_message_terminal.h"

#include <ctime>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "share/define.h"

namespace TankBattle {
class GameCore;
struct PlayerInput;
enum class Faction;
}

namespace wukong {
namespace pb {
class BattleKcpInputUpload;
class BattlePlayerFrameInput;
}

class BattleRoom {
public:
    static const int kFactionCount = 4;

    struct PlayerSlot {
        uint64_t roleId = 0;
        std::string sessionToken;
        std::string combatPayload;
        ServerId lobbyServerId = 0;
        bool hasAuthed = false;
        std::shared_ptr<corpc::MessageTerminal::Connection> conn;
        std::time_t offlineSince_ = 0;
        std::time_t assignedAt_ = 0;
        uint32_t gamePlayerId = 0;
        uint32_t gameTankId = 0;
        int preferredFaction = -1; // 客户端所选；<0 表示未指定
        int reservedFaction = -1;  // 分配时占用的阵营席位（用于 humanCount）
        int gameFaction = 0;       // TankBattle::Faction as int（进模拟后最终值）
    };

    BattleRoom(uint64_t roomId, pb::BattleRoomMode mode, uint32_t battleDefId,
               const BattleRoomTypeDef &typeDef, std::vector<PlayerSlot> &&players);
    ~BattleRoom();

    uint64_t roomId() const { return roomId_; }
    pb::BattleRoomMode mode() const { return mode_; }
    uint32_t battleDefId() const { return battleDefId_; }
    uint32_t maxPlayers() const { return maxPlayers_; }
    uint32_t slotsPerFaction() const { return slotsPerFaction_; }
    uint32_t syncFrameIndex() const { return syncFrameIndex_; }

    bool matchesTemplate(pb::BattleRoomMode m, uint32_t defId) const {
        return mode_ == m && battleDefId_ == defId;
    }

    // 该阵营是否仍可加入（humanCount + 加入窗口 + 未超时结束）
    bool canJoinFaction(int faction, std::time_t now) const;
    bool isJoinWindowOpen(std::time_t now) const;
    bool isBattleTimeUp(std::time_t now) const;
    int humanCount(int faction) const;

    ServerId getPlayerLobbyServerId(uint64_t roleId) const;
    void forEachPlayer(const std::function<void(const PlayerSlot &)> &fn) const;

    bool tryAuth(uint64_t roleId, const std::string &token, const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    void detachConnectionForRole(uint64_t roleId, const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);

    bool leaveBattle(uint64_t roleId, const std::string &token);
    bool forceRemovePlayer(uint64_t roleId);

    struct PendingControlEvent {
        pb::BattleControlEventType type = pb::BATTLE_CONTROL_NONE;
        uint64_t roleId = 0;
        uint32_t playerId = 0;
        uint32_t tankId = 0;
        int faction = 0;
        std::string name;
    };
    bool addJoiningPlayer(const pb::BattlePlayerInitData &player, const std::string &token, ServerId lobbyServerId,
                          std::time_t assignedAt);

    /// 断线重连：角色已在房内时换发 token，保留 gamePlayerId/坦克绑定
    bool rebindPlayerForReconnect(uint64_t roleId, const std::string &token, ServerId lobbyServerId,
                                  std::time_t assignedAt);

    bool hasRole(uint64_t roleId) const { return players_.find(roleId) != players_.end(); }

    bool shouldDestroy(std::time_t now, uint32_t emptySeconds) const;

    void tickFrameSync();

    void submitInputUpload(uint64_t roleId, const pb::BattleKcpInputUpload &msg);

    void expireOfflinePlayers(std::time_t now, uint32_t offlineKickSec, std::vector<std::pair<uint64_t, ServerId>> *removed);
    void expireUnauthedPlayers(std::time_t now, uint32_t verifySec, std::vector<std::pair<uint64_t, ServerId>> *removed);

    static int normalizeFaction(int faction, uint64_t roleId);

private:
    bool hasAnyOnlineAuthedPlayer() const;
    void noteNoPlayersSince();
    void sendSnapshotTo(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    void broadcastSnapshot();
    std::shared_ptr<pb::BattleRoomSnapshot> buildSnapshotProto();
    void queueControlEvent(const PendingControlEvent &ev);

    void ensureSimulationInitialized();
    bool addPlayerToSimulation(uint64_t roleId);
    void removePlayerFromSimulation(uint64_t roleId);
    void releaseFactionSeat(int faction);
    TankBattle::Faction resolveFactionForRole(uint64_t roleId) const;
    static void fillProtoInput(const TankBattle::PlayerInput &src, pb::BattlePlayerFrameInput *dst, uint64_t roleId);

    std::unordered_map<uint64_t, PlayerSlot>::iterator erasePlayerSlot(
        std::unordered_map<uint64_t, PlayerSlot>::iterator it);

    uint64_t roomId_;
    uint32_t syncFrameIndex_ = 0;
    std::time_t noPlayersSince_ = 0;
    pb::BattleRoomMode mode_;
    uint32_t battleDefId_;
    uint32_t maxPlayers_;
    uint32_t slotsPerFaction_ = 2;
    uint32_t battleDurationSec_ = 300;
    uint32_t joinWindowSec_ = 120;
    std::unordered_map<uint64_t, PlayerSlot> players_;

    // 各阵营已占用真人席位（分配时 +1，离开/超时 -1）；不向 GameCore 查询
    int humanCount_[kFactionCount] = {};

    std::unique_ptr<TankBattle::GameCore> gameCore_;
    bool simulationStarted_ = false;
    std::time_t battleStartedAt_ = 0;
    uint32_t randomSeed_ = 1;
    std::map<uint32_t, TankBattle::PlayerInput> pendingFrameInputs_;
    std::vector<PendingControlEvent> pendingControlEvents_;
    // 兼容旧逻辑：Auth 已立即发 Snapshot，tick 中清空即可
    std::vector<uint64_t> pendingFullSnapshotRoleIds_;
};

}

#endif

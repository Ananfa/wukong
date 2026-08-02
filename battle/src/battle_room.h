/*
 * 单个帧同步战斗房间（单人/多人共用）
 *
 * 匹配进房：Lobby RPC 登记玩家后客户端 KCP 鉴权；鉴权成功后才计为「在战局中」推进帧同步。
 * 掉线：断开 conn，hasAuthed 仍为 true；超过 disconnectTimeout 未重连则从房间移除并通知 Lobby。
 * 未鉴权席位：超过 verifyTimeout 未 KCP 鉴权则移除并通知 Lobby（取消等待进房）。
 * 主动离开：删席。
 * 房间销毁：players_ 为空时起算，持续 roomEmptyDestroySec 后销毁。
 */
#ifndef wukong_battle_room_h
#define wukong_battle_room_h

#include "battle_service.pb.h"
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
        int preferredFaction = -1; // 客户端所选；<0 表示未指定
        int gameFaction = 0; // TankBattle::Faction as int（进模拟后最终值）
    };

    BattleRoom(uint64_t roomId, pb::BattleRoomMode mode, uint32_t battleDefId, uint32_t maxPlayers,
               std::vector<PlayerSlot> &&players);
    ~BattleRoom();

    uint64_t roomId() const { return roomId_; }
    pb::BattleRoomMode mode() const { return mode_; }
    uint32_t battleDefId() const { return battleDefId_; }
    uint32_t maxPlayers() const { return maxPlayers_; }
    uint32_t syncFrameIndex() const { return syncFrameIndex_; }

    bool matchesTemplate(pb::BattleRoomMode m, uint32_t defId) const {
        return mode_ == m && battleDefId_ == defId;
    }
    bool canAddPlayer() const { return players_.size() < maxPlayers_; }

    ServerId getPlayerLobbyServerId(uint64_t roleId) const;
    void forEachPlayer(const std::function<void(const PlayerSlot &)> &fn) const;

    bool tryAuth(uint64_t roleId, const std::string &token, const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    void detachConnectionForRole(uint64_t roleId, const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);

    bool leaveBattle(uint64_t roleId, const std::string &token);
    bool forceRemovePlayer(uint64_t roleId);
    bool addJoiningPlayer(const pb::BattlePlayerInitData &player, const std::string &token, ServerId lobbyServerId,
                          std::time_t assignedAt);

    bool shouldDestroy(std::time_t now, uint32_t emptySeconds) const;

    void tickFrameSync();

    void submitInputUpload(uint64_t roleId, const pb::BattleKcpInputUpload &msg);

    void expireOfflinePlayers(std::time_t now, uint32_t offlineKickSec, std::vector<std::pair<uint64_t, ServerId>> *removed);
    void expireUnauthedPlayers(std::time_t now, uint32_t verifySec, std::vector<std::pair<uint64_t, ServerId>> *removed);

private:
    bool hasAnyOnlineAuthedPlayer() const;
    void noteNoPlayersSince();
    void sendSnapshotTo(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);

    void ensureSimulationInitialized();
    bool addPlayerToSimulation(uint64_t roleId);
    void removePlayerFromSimulation(uint64_t roleId);
    TankBattle::Faction resolveFactionForRole(uint64_t roleId) const;
    static void fillProtoInput(const TankBattle::PlayerInput &src, pb::BattlePlayerFrameInput *dst, uint64_t roleId);

    uint64_t roomId_;
    uint32_t syncFrameIndex_ = 0;
    std::time_t noPlayersSince_ = 0;
    pb::BattleRoomMode mode_;
    uint32_t battleDefId_;
    uint32_t maxPlayers_;
    std::unordered_map<uint64_t, PlayerSlot> players_;

    std::unique_ptr<TankBattle::GameCore> gameCore_;
    bool simulationStarted_ = false;
    uint32_t randomSeed_ = 1;
    std::map<uint32_t, TankBattle::PlayerInput> pendingFrameInputs_;
};

}

#endif

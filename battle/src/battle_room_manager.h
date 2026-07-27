/*
 * 战斗房间管理：RPC 创建/匹配进房/中途加入、KCP 鉴权/主动离开、断线踢出、未鉴权超时、空闲销毁
 *
 * rooms_ 无锁：访问均在 Battle 进程同一条协程调度链（主 IO worker）上执行。
 */
#ifndef wukong_battle_room_manager_h
#define wukong_battle_room_manager_h

#include "battle_service.pb.h"
#include "corpc_message_terminal.h"
#include "share/define.h"

#include <ctime>
#include <memory>
#include <string>
#include <unordered_map>

namespace wukong {

class BattleRoom;

class BattleRoomManager {
public:
    static BattleRoomManager &Instance() {
        static BattleRoomManager inst;
        return inst;
    }

    void init();

    bool assignFromRequest(const pb::RequestBattleAssignmentRequest &req, pb::RequestBattleAssignmentResponse &rsp);

    void onPlayerKcpDisconnected(uint64_t roleId, uint64_t roomId,
                                 const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);

    bool authPlayerInRoom(uint64_t roomId, uint64_t roleId, const std::string &sessionToken,
                          const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    bool leavePlayerByClient(uint64_t roomId, uint64_t roleId, const std::string &sessionToken);
    bool removePlayer(uint64_t roomId, uint64_t roleId);

    bool submitInputUpload(uint64_t roomId, uint64_t roleId, const pb::BattleKcpInputUpload &msg);

    void tickIdleRooms();

    void tickAllRoomsFrameSync();

    static void *idleTickRoutine(void *arg);
    static void *frameSyncTickRoutine(void *arg);

private:
    BattleRoomManager() = default;

    std::string makeToken(uint64_t roleId, uint64_t roomId);

    std::shared_ptr<BattleRoom> findMatchableRoom(pb::BattleRoomMode mode, uint32_t battleDefId);

    void notifyLobbyPlayerEnterBattle(ServerId lobbySid, uint64_t roleId, const std::shared_ptr<BattleRoom> &room);
    void notifyLobbyPlayerLeaveBattle(ServerId lobbySid, uint64_t roleId);
    void notifyLobbyPlayersRoomDestroyed(const std::shared_ptr<BattleRoom> &room);

    bool inited_ = false;
    uint64_t nextRoomId_ = 1;
    std::unordered_map<uint64_t, std::shared_ptr<BattleRoom>> rooms_;
};

#define g_BattleRoomManager BattleRoomManager::Instance()

}

#endif

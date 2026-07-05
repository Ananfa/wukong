/*
 * 角色战斗运行时状态：
 * - 未认证连接超时管理
 * - 玩家所在房间与连接映射（用于跨房切换与重复认证判断）
 */
#ifndef wukong_battle_role_manager_h
#define wukong_battle_role_manager_h

#include "battle_sync.pb.h"
#include "corpc_message_terminal.h"
#include "timelink.h"

#include <cstdint>
#include <ctime>
#include <map>
#include <memory>
#include <unordered_map>

namespace wukong {

class BattleRoleManager {
public:
    static BattleRoleManager &Instance() {
        static BattleRoleManager inst;
        return inst;
    }
    void init();

    void onConnectionOpened(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    void onConnectionClosed(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    void cleanupUnauthedConnections(std::time_t now, uint32_t authTimeoutSec);
    bool onKcpAuth(const std::shared_ptr<pb::BattleKcpAuth> &auth,
                   const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    bool onKcpLeaveRoom(const std::shared_ptr<pb::BattleKcpLeaveRoom> &req);

    void onAuthSucceeded(uint64_t roleId, uint64_t roomId, const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);
    void onRoleRemoved(uint64_t roleId);

    bool isSameRoomSameConn(uint64_t roleId, uint64_t roomId,
                            const std::shared_ptr<corpc::MessageTerminal::Connection> &conn) const;
    bool getRoleRoom(uint64_t roleId, uint64_t *roomId) const;

private:
    BattleRoleManager() = default;
    static void *cleanupRoutine(void *arg);
    static constexpr uint32_t kConnAuthTimeoutSec = 5;
    using PendingAuthLink = TimeLink<corpc::MessageTerminal::Connection>;
    using PendingAuthNode = PendingAuthLink::Node;
    void removeUnauthedConn(const std::shared_ptr<corpc::MessageTerminal::Connection> &conn);

    struct RoleBattleState {
        uint64_t roomId = 0;
        std::shared_ptr<corpc::MessageTerminal::Connection> conn;
    };

    // 未认证连接：时间队列 + 指针索引，超时清理仅检查队头
    PendingAuthLink pendingAuthLink_;
    std::map<corpc::MessageTerminal::Connection *, PendingAuthNode *> pendingAuthNodeMap_;
    // 已鉴权连接(Connection*) -> roleId（断线时 O(1) 定位玩家）
    std::unordered_map<uintptr_t, uint64_t> authFdToRoleId_;
    std::unordered_map<uint64_t, RoleBattleState> roleStates_;
};

#define g_BattleRoleManager BattleRoleManager::Instance()

}

#endif

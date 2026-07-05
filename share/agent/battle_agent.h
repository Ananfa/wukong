/*
 * Lobby -> Battle RPC（匹配进房等）
 */
#ifndef wukong_battle_agent_h
#define wukong_battle_agent_h

#include "agent.h"
#include "share/const.h"
#include "battle_service.pb.h"

namespace wukong {

class BattleAgent : public Agent {
public:
    explicit BattleAgent(corpc::RpcClient *client) : Agent(SERVER_TYPE_BATTLE, client) {}
    ~BattleAgent() override = default;

    void setStub(const pb::ServerInfo &serverInfo) override;
    void shutdown() override;

    bool requestBattleAssignment(ServerId sid, const pb::RequestBattleAssignmentRequest &req,
                                 pb::RequestBattleAssignmentResponse *resp);
    void removeBattlePlayer(ServerId sid, const pb::RemoveBattlePlayerRequest &req);
};

}

#endif

#include "AiPerception.h"
#include "../Common/Constants.h"
#include "../Common/FixedMath.h"
#include "../ObstacleWall.h"
#include "../TankLogicView.h"
#include <limits>

namespace TankBattle
{
    void AiPerception::Refresh(
        const Tank& self,
        const std::vector<std::shared_ptr<Tank>>& allTanks,
        const std::vector<ObstacleWall>& obstacles,
        AiBlackboard& bb,
        AiTankMemory& memory)
    {
        const TankLogicView selfView = self.GetLogicView();
        bb.selfId = selfView.id;
        bb.selfHp = selfView.hp;
        bb.selfMaxHp = selfView.maxHp;
        bb.canFire = self.CanFire();
        bb.abilityReady = self.IsAbilityReady();
        bb.hasTarget = false;
        bb.hasLineOfSight = false;
        bb.targetId = 0;
        bb.distToTargetSq = 0;
        bb.targetHp = 0;
        bb.targetMaxHp = 0;

        const Pos clearancePos = static_cast<Pos>(kAiObstacleClearancePosValue);
        int64_t minDistSq = std::numeric_limits<int64_t>::max();
        uint32_t bestTargetId = 0;
        for (const auto& other : allTanks)
        {
            if (!other || other->GetId() == self.GetId()) continue;
            if (!AreHostileFactions(self.GetFaction(), other->GetFaction())) continue;
            if (!other->IsAlive()) continue;

            const TankLogicView otherView = other->GetLogicView();
            if (!HasLineOfSightFixed(
                    selfView.position.x,
                    selfView.position.y,
                    otherView.position.x,
                    otherView.position.y,
                    obstacles,
                    clearancePos))
                continue;

            const int64_t distSq = FixedDistanceSquared(selfView.position, otherView.position);
            if (distSq < minDistSq ||
                (distSq == minDistSq && other->GetId() < bestTargetId))
            {
                minDistSq = distSq;
                bestTargetId = other->GetId();
                bb.targetId = other->GetId();
                bb.distToTargetSq = distSq;
                bb.targetHp = otherView.hp;
                bb.targetMaxHp = otherView.maxHp;
                bb.hasTarget = true;
                bb.hasLineOfSight = true;
            }
        }

        switch (selfView.type)
        {
        case TankType::Tiger:
            bb.engageRangePos = static_cast<Pos>(kAiEngageRangeTigerPosValue);
            bb.retreatRangePos = static_cast<Pos>(kAiRetreatRangeTigerPosValue);
            break;
        case TankType::KV2:
            bb.engageRangePos = static_cast<Pos>(kAiEngageRangeKv2PosValue);
            bb.retreatRangePos = static_cast<Pos>(kAiRetreatRangeKv2PosValue);
            break;
        default:
            bb.engageRangePos = static_cast<Pos>(kAiEngageRangeDefaultPosValue);
            bb.retreatRangePos = static_cast<Pos>(kAiRetreatRangeDefaultPosValue);
            break;
        }
        bb.engageRangeSq = PosDistanceSquared(bb.engageRangePos);
        bb.retreatRangeSq = PosDistanceSquared(bb.retreatRangePos);

        (void)memory;
    }
}

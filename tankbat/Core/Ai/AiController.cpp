#include "AiController.h"
#include "AiPerception.h"
#include "AiSteering.h"
#include "../ObstacleWall.h"
#include "Constants.h"

namespace TankBattle
{
    AiController::AiController()
        : m_root(BuildDefaultTankBehaviorTree())
    {
    }

    AiTankMemory& AiController::GetOrCreateMemory(uint32_t tankId, uint32_t frameIndex, const GameRng& rng)
    {
        auto it = m_memories.find(tankId);
        if (it == m_memories.end())
        {
            AiTankMemory memory;
            memory.wanderHeading = static_cast<Angle>(rng.UniformAngleUnits(
                frameIndex,
                RngPurpose::AiInitialHeading,
                tankId));
            it = m_memories.emplace(tankId, memory).first;
        }
        return it->second;
    }

    AiIntent AiController::Tick(
        Tank& self,
        const std::vector<std::shared_ptr<Tank>>& allTanks,
        const std::vector<ObstacleWall>& obstacles,
        const NavigationGrid& navGrid,
        uint32_t frameIndex,
        const GameRng& rng)
    {
        AiIntent intent;
        AiBlackboard bb;
        AiTankMemory& memory = GetOrCreateMemory(self.GetId(), frameIndex, rng);

        AiPerception::Refresh(self, allTanks, obstacles, bb, memory);

        BTContext ctx;
        ctx.self = &self;
        ctx.bb = &bb;
        ctx.intent = &intent;
        ctx.memory = &memory;
        ctx.allTanks = &allTanks;
        ctx.navGrid = navGrid.IsValid() ? &navGrid : nullptr;
        ctx.frameIndex = frameIndex;
        ctx.gameRng = &rng;

        if (m_root)
            m_root->Tick(ctx);

        AiSteering::ApplyObstacleAvoidance(intent, self, obstacles);

        return intent;
    }

    void AiController::Clear()
    {
        m_memories.clear();
    }
}

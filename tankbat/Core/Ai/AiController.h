#pragma once
#include "AiIntent.h"
#include "AiTypes.h"
#include "NavigationGrid.h"
#include "bt/TankBehaviorTree.h"
#include "../ObstacleWall.h"
#include "../../Common/GameRng.h"
#include "../../Common/Types.h"
#include "Tank.h"
#include <cstdint>
#include <memory>
#include <map>
#include <vector>

namespace TankBattle
{
    class AiController
    {
    public:
        AiController();

        AiIntent Tick(
            Tank& self,
            const std::vector<std::shared_ptr<Tank>>& allTanks,
            const std::vector<ObstacleWall>& obstacles,
            const NavigationGrid& navGrid,
            uint32_t frameIndex,
            const GameRng& rng);

        void Clear();
        void RemoveMemory(uint32_t tankId);

        void ExportMemories(std::vector<AiTankMemorySnapshot>& out) const;
        void ApplyMemories(const std::vector<AiTankMemorySnapshot>& memories);

    private:
        AiTankMemory& GetOrCreateMemory(uint32_t tankId, uint32_t frameIndex, const GameRng& rng);

        std::unique_ptr<BTNode> m_root;
        std::map<uint32_t, AiTankMemory> m_memories;
    };
}

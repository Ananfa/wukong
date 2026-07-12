#pragma once
#include "AiBlackboard.h"
#include "AiTypes.h"
#include "Tank.h"
#include "../ObstacleWall.h"
#include <vector>
#include <memory>

namespace TankBattle
{
    class AiPerception
    {
    public:
        static void Refresh(
            const Tank& self,
            const std::vector<std::shared_ptr<Tank>>& allTanks,
            const std::vector<ObstacleWall>& obstacles,
            AiBlackboard& bb,
            AiTankMemory& memory);
    };
}

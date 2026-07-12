#pragma once
#include "../ObstacleWall.h"
#include "AiIntent.h"
#include "Tank.h"
#include <vector>

namespace TankBattle
{
    class AiSteering
    {
    public:
        static void ApplyObstacleAvoidance(
            AiIntent& intent,
            const Tank& self,
            const std::vector<ObstacleWall>& obstacles);
    };
}

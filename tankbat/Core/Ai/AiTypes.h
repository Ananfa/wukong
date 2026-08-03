#pragma once
#include "../../Common/FixedMath.h"
#include "../../Common/Types.h"
#include "../../Common/AngleLUT.h"
#include <cstdint>
#include <vector>

namespace TankBattle
{
    struct AiTankMemory
    {
        Angle wanderHeading = 0;
        int strafeSign = 1;
        int strafeSwitchFramesRemaining = 0;
        uint32_t wanderGoalSerial = 0;

        std::vector<FixedVec2> pathWaypoints;
        size_t pathWaypointIndex = 0;
        FixedVec2 pathGoal;
        uint32_t pathTargetId = 0;
        AiMoveMode pathMoveMode = AiMoveMode::None;
        int pathRecalcFramesRemaining = 0;
        FixedVec2 wanderPathGoal;
        int wanderPathFramesRemaining = 0;
    };

}

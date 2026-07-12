#pragma once
#include "AiMovement.h"
#include "NavigationGrid.h"
#include "bt/BTNode.h"

namespace TankBattle
{
    class AiPathFollower
    {
    public:
        static bool MoveToward(
            BTContext& ctx,
            const NavigationGrid* grid,
            const FixedVec2& goalFixed,
            AiMoveMode mode,
            int32_t speedScaleQ15,
            uint32_t relatedTargetId = 0);

        static bool WanderOnGrid(BTContext& ctx, const NavigationGrid* grid, int32_t speedScaleQ15);
    };
}

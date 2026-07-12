#pragma once
#include "AiBlackboard.h"
#include "AiIntent.h"
#include "../Common/AngleLUT.h"
#include "../Common/Constants.h"

namespace TankBattle
{
    inline void ApplyMoveHeading(
        AiIntent& intent,
        const AiBlackboard& /*bb*/,
        AiMoveMode mode,
        Angle heading,
        int32_t speedScaleQ15)
    {
        intent.moveMode = mode;
        intent.moveHeading = heading;
        intent.speedScaleQ15 = speedScaleQ15;
    }
}

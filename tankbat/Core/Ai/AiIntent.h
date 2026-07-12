#pragma once
#include "../../Common/Constants.h"
#include "../../Common/AngleLUT.h"
#include "../../Common/Types.h"

namespace TankBattle
{
    // AI 单帧决策输出，由 GameCore 执行（移动 / 开火 / 技能）
    struct AiIntent
    {
        uint32_t targetId = 0;
        AiMoveMode moveMode = AiMoveMode::None;
        Angle moveHeading = 0;
        int32_t speedScaleQ15 = 0;
        bool wantFire = false;
        bool wantAbility = false;
    };
}

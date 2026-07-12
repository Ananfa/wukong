#pragma once
#include "../Common/FixedMath.h"
#include <cstdint>

namespace TankBattle
{
    // 感知数据：Tick 前由 AiPerception 填充，BT 节点只读
    struct AiBlackboard
    {
        uint32_t selfId = 0;
        uint32_t targetId = 0;
        int64_t distToTargetSq = 0;
        int32_t selfHp = 0;
        int32_t selfMaxHp = 0;
        int32_t targetHp = 0;
        int32_t targetMaxHp = 0;
        bool canFire = false;
        bool abilityReady = false;
        bool hasTarget = false;
        bool hasLineOfSight = false;

        Pos engageRangePos = 0;
        Pos retreatRangePos = 0;
        int64_t engageRangeSq = 0;
        int64_t retreatRangeSq = 0;
    };
}

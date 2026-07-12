#pragma once

#include "Constants.h"
#include "FixedMath.h"
#include <cstdint>

namespace TankBattle
{
    // 0..65535 对应一整圈，与 kRngAngleUnits 一致
    using Angle = uint16_t;

    constexpr int kAngleSinCosScale = 32767;
    constexpr Angle kAngleHalfTurn = 32768;
    constexpr Angle kAngleQuarterTurn = 16384;
    constexpr Angle kAngleProbeOffset = 8192;   // 45°
    constexpr Angle kAngleProbeOffset90 = 16384; // 90°
    constexpr Angle kAngleProbeOffset135 = 24576; // 135°

    // 初始化 sin/cos LUT（CORDIC，跨平台一致）
    void InitAngleLut();

    inline Angle AddAngle(Angle angle, int delta)
    {
        return static_cast<Angle>(static_cast<uint32_t>(angle) + static_cast<uint32_t>(delta));
    }

    int32_t SinQ15(Angle angle);
    int32_t CosQ15(Angle angle);

    // 由 Pos 平面向量求朝向（dy, dx 对应 y/x）
    Angle Atan2Pos(int32_t dy, int32_t dx);

    // 仅用于 Unity 快照边界换算
    float AngleToRadians(Angle angle);

    FixedVel2 VelFromAngleVelPerFrame(Angle angle, Vel speedVelPerFrame);
    FixedVel2 VelFromAngleVelScaledQ15(Angle angle, Vel speedVelPerFrame, int32_t scaleQ15);

    void OffsetFixedPosition(
        const FixedVec2& origin,
        Angle direction,
        Pos offsetPos,
        FixedVec2& outPosition);
}

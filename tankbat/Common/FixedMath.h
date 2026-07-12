#pragma once

#include "Constants.h"
#include <cstdint>

namespace TankBattle
{
    using Pos = int32_t;
    using Vel = int32_t;

    constexpr int kPosScale = kRngWorldSubunitsPerUnit;
    constexpr int kRecoilDecayNumerator = 11;
    constexpr int kRecoilDecayDenominator = 15;

    struct FixedVec2
    {
        Pos x = 0;
        Pos y = 0;
    };

    struct FixedVel2
    {
        Vel x = 0;
        Vel y = 0;
    };

    inline float PosToWorld(Pos pos)
    {
        return static_cast<float>(pos) / static_cast<float>(kPosScale);
    }

    constexpr Vel WorldSpeedToVelPerFrameInt(int speedPerSec)
    {
        return static_cast<Vel>(
            (static_cast<int64_t>(speedPerSec) * kPosScale * 1000 + 500)
            / (static_cast<int64_t>(kLogicFramesPerSecond) * 1000));
    }

    inline float VelPerFrameToWorldSpeed(Vel velPerFrame)
    {
        return static_cast<float>(velPerFrame)
            * static_cast<float>(kLogicFramesPerSecond)
            / static_cast<float>(kPosScale);
    }

    inline int64_t FixedDistanceSquared(const FixedVec2& a, const FixedVec2& b)
    {
        const int64_t dx = static_cast<int64_t>(a.x) - static_cast<int64_t>(b.x);
        const int64_t dy = static_cast<int64_t>(a.y) - static_cast<int64_t>(b.y);
        return dx * dx + dy * dy;
    }

    inline int64_t PosDistanceSquared(Pos distancePos)
    {
        return static_cast<int64_t>(distancePos) * static_cast<int64_t>(distancePos);
    }

    inline Pos TankCollisionRadiusPos(Pos sizePos)
    {
        return static_cast<Pos>(
            (static_cast<int64_t>(sizePos) * kTankCollisionRadiusNumerator)
            / kTankCollisionRadiusDenominator);
    }

    inline int32_t Isqrt64(int64_t value)
    {
        if (value <= 0)
            return 0;

        int64_t x = value;
        int64_t y = (x + 1) / 2;
        while (y < x)
        {
            x = y;
            y = (x + value / x) / 2;
        }
        return static_cast<int32_t>(x);
    }

    inline FixedVel2 ScaleVelPerFrame(FixedVel2 vel, int numerator, int denominator)
    {
        if (denominator == 0)
            return {0, 0};
        return {
            static_cast<Vel>((static_cast<int64_t>(vel.x) * numerator) / denominator),
            static_cast<Vel>((static_cast<int64_t>(vel.y) * numerator) / denominator)
        };
    }

    inline void ApplyRecoilDecay(FixedVel2& recoilVel)
    {
        recoilVel.x = static_cast<Vel>(
            (static_cast<int64_t>(recoilVel.x) * kRecoilDecayNumerator) / kRecoilDecayDenominator);
        recoilVel.y = static_cast<Vel>(
            (static_cast<int64_t>(recoilVel.y) * kRecoilDecayNumerator) / kRecoilDecayDenominator);
    }

    inline bool RecoilVelIsActive(const FixedVel2& recoilVel)
    {
        return recoilVel.x != 0 || recoilVel.y != 0;
    }
}

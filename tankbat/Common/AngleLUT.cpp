#include "AngleLUT.h"

namespace TankBattle
{
    namespace
    {
        constexpr int kAngleLutSize = 4096;
        constexpr int kAngleLutShift = 4;
        constexpr int kCordicIterations = 16;

        constexpr uint16_t kCordicAtanTable[kCordicIterations] = {
            8192, 4836, 2555, 1297, 651, 326, 163, 81, 41, 20, 10, 5, 3, 1, 1, 0
        };

        int32_t s_sinLut[kAngleLutSize];
        int32_t s_cosLut[kAngleLutSize];
        bool s_initialized = false;

        void CordicSinCosQuarter(int32_t quarterAngle, int32_t& outSin, int32_t& outCos)
        {
            int32_t x = 19898;
            int32_t y = 0;
            int32_t z = quarterAngle;

            for (int i = 0; i < kCordicIterations; ++i)
            {
                const int32_t ySh = y >> i;
                const int32_t xSh = x >> i;
                if (z >= 0)
                {
                    x -= ySh;
                    y += xSh;
                    z -= static_cast<int32_t>(kCordicAtanTable[i]);
                }
                else
                {
                    x += ySh;
                    y -= xSh;
                    z += static_cast<int32_t>(kCordicAtanTable[i]);
                }
            }

            outSin = y;
            outCos = x;
        }

        void CordicSinCos(Angle angle, int32_t& outSin, int32_t& outCos)
        {
            const uint32_t a = static_cast<uint32_t>(angle);
            int32_t sinQ = 0;
            int32_t cosQ = 0;

            if (a < static_cast<uint32_t>(kAngleQuarterTurn))
            {
                CordicSinCosQuarter(static_cast<int32_t>(a), sinQ, cosQ);
            }
            else if (a < static_cast<uint32_t>(kAngleHalfTurn))
            {
                CordicSinCosQuarter(static_cast<int32_t>(kAngleHalfTurn - a), sinQ, cosQ);
                cosQ = -cosQ;
            }
            else if (a < static_cast<uint32_t>(kAngleHalfTurn + kAngleQuarterTurn))
            {
                CordicSinCosQuarter(static_cast<int32_t>(a - kAngleHalfTurn), sinQ, cosQ);
                sinQ = -sinQ;
                cosQ = -cosQ;
            }
            else
            {
                CordicSinCosQuarter(static_cast<int32_t>(kRngAngleUnits - a), sinQ, cosQ);
                sinQ = -sinQ;
            }

            outSin = sinQ;
            outCos = cosQ;
        }

        void BuildAngleLut()
        {
            for (int i = 0; i < kAngleLutSize; ++i)
            {
                const Angle angle = static_cast<Angle>(i << kAngleLutShift);
                int32_t sinQ15 = 0;
                int32_t cosQ15 = 0;
                CordicSinCos(angle, sinQ15, cosQ15);
                if (sinQ15 > kAngleSinCosScale)
                    sinQ15 = kAngleSinCosScale;
                else if (sinQ15 < -kAngleSinCosScale)
                    sinQ15 = -kAngleSinCosScale;
                if (cosQ15 > kAngleSinCosScale)
                    cosQ15 = kAngleSinCosScale;
                else if (cosQ15 < -kAngleSinCosScale)
                    cosQ15 = -kAngleSinCosScale;
                s_sinLut[i] = sinQ15;
                s_cosLut[i] = cosQ15;
            }
            s_initialized = true;
        }
    }

    void InitAngleLut()
    {
        if (!s_initialized)
            BuildAngleLut();
    }

    int32_t SinQ15(Angle angle)
    {
        InitAngleLut();
        return s_sinLut[angle >> kAngleLutShift];
    }

    int32_t CosQ15(Angle angle)
    {
        InitAngleLut();
        return s_cosLut[angle >> kAngleLutShift];
    }

    Angle Atan2Pos(int32_t dy, int32_t dx)
    {
        if (dy == 0 && dx == 0)
            return 0;

        uint32_t lo = 0;
        uint32_t hi = static_cast<uint32_t>(kRngAngleUnits) - 1;
        for (int iter = 0; iter < 18; ++iter)
        {
            const uint32_t mid = (lo + hi) >> 1;
            const int32_t sinMid = SinQ15(static_cast<Angle>(mid));
            const int32_t cosMid = CosQ15(static_cast<Angle>(mid));
            const int64_t cross =
                static_cast<int64_t>(dx) * sinMid - static_cast<int64_t>(dy) * cosMid;
            if (cross > 0)
                hi = mid;
            else
                lo = mid + 1;
        }
        return static_cast<Angle>(lo);
    }

    float AngleToRadians(Angle angle)
    {
        return AngleUnitsToRadians(static_cast<int>(angle));
    }

    FixedVel2 VelFromAngleVelPerFrame(Angle angle, Vel speedVelPerFrame)
    {
        return {
            static_cast<Vel>((static_cast<int64_t>(CosQ15(angle)) * speedVelPerFrame) / kAngleSinCosScale),
            static_cast<Vel>((static_cast<int64_t>(SinQ15(angle)) * speedVelPerFrame) / kAngleSinCosScale)
        };
    }

    FixedVel2 VelFromAngleVelScaledQ15(Angle angle, Vel speedVelPerFrame, int32_t scaleQ15)
    {
        const int64_t scaledSpeed =
            (static_cast<int64_t>(speedVelPerFrame) * scaleQ15) / kAngleSinCosScale;
        return {
            static_cast<Vel>((static_cast<int64_t>(CosQ15(angle)) * scaledSpeed) / kAngleSinCosScale),
            static_cast<Vel>((static_cast<int64_t>(SinQ15(angle)) * scaledSpeed) / kAngleSinCosScale)
        };
    }

    void OffsetFixedPosition(
        const FixedVec2& origin,
        Angle direction,
        Pos offsetPos,
        FixedVec2& outPosition)
    {
        outPosition.x = origin.x + static_cast<Pos>(
            (static_cast<int64_t>(CosQ15(direction)) * offsetPos) / kAngleSinCosScale);
        outPosition.y = origin.y + static_cast<Pos>(
            (static_cast<int64_t>(SinQ15(direction)) * offsetPos) / kAngleSinCosScale);
    }
}

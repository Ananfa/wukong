#pragma once

#include "Constants.h"
#include <cstdint>
#include <random>

namespace TankBattle
{
    enum class RngPurpose : uint32_t
    {
        TankInitialRotation = 1,
        TankRespawnRotation = 2,
        TankTypeRoll = 3,
        SpawnPosition = 4,
        RespawnPosition = 5,
        UnblockedPosition = 6,
        FactionSpawnPosition = 7,
        AiInitialHeading = 10,
        AiWanderTurn = 11,
        AiWanderGoal = 12,
        AiStrafeSign = 13,
    };

    class GameRng
    {
    public:
        void SetSeed(uint32_t seed) { gameSeed_ = seed; }
        uint32_t GetSeed() const { return gameSeed_; }

        static uint32_t HashCombine(uint32_t hash, uint32_t value)
        {
            hash ^= value + 0x9e3779b9u + (hash << 6) + (hash >> 2);
            return hash;
        }

        static uint32_t MakeSubSeed(
            uint32_t gameSeed,
            uint32_t frameIndex,
            RngPurpose purpose,
            uint32_t entityId,
            uint32_t salt = 0)
        {
            uint32_t hash = gameSeed;
            hash = HashCombine(hash, frameIndex);
            hash = HashCombine(hash, static_cast<uint32_t>(purpose));
            hash = HashCombine(hash, entityId);
            hash = HashCombine(hash, salt);
            return hash;
        }

        std::mt19937 MakeEngine(
            uint32_t frameIndex,
            RngPurpose purpose,
            uint32_t entityId,
            uint32_t salt = 0) const
        {
            return std::mt19937(MakeSubSeed(gameSeed_, frameIndex, purpose, entityId, salt));
        }

        int UniformInt(
            uint32_t frameIndex,
            RngPurpose purpose,
            uint32_t entityId,
            int minInclusive,
            int maxExclusive,
            uint32_t salt = 0) const
        {
            std::uniform_int_distribution<int> dist(minInclusive, maxExclusive - 1);
            auto engine = MakeEngine(frameIndex, purpose, entityId, salt);
            return dist(engine);
        }

        // [0, kRngAngleUnits) 离散角度单元，用 AngleUnitsToRadians 转弧度
        int UniformAngleUnits(
            uint32_t frameIndex,
            RngPurpose purpose,
            uint32_t entityId,
            uint32_t salt = 0) const
        {
            return UniformInt(frameIndex, purpose, entityId, 0, kRngAngleUnits, salt);
        }

        // [minSubunitsInclusive, maxSubunitsExclusive) 世界坐标子单位
        int UniformWorldSubunits(
            uint32_t frameIndex,
            RngPurpose purpose,
            uint32_t entityId,
            int minSubunitsInclusive,
            int maxSubunitsExclusive,
            uint32_t salt = 0) const
        {
            return UniformInt(
                frameIndex,
                purpose,
                entityId,
                minSubunitsInclusive,
                maxSubunitsExclusive,
                salt);
        }

        uint32_t SubSeed(
            uint32_t frameIndex,
            RngPurpose purpose,
            uint32_t entityId,
            uint32_t salt = 0) const
        {
            return MakeSubSeed(gameSeed_, frameIndex, purpose, entityId, salt);
        }

    private:
        uint32_t gameSeed_ = 1;
    };
}

#include "ObstacleWall.h"
#include "../Common/Constants.h"

namespace TankBattle
{
    namespace
    {
        inline Pos ClampPos(Pos value, Pos minValue, Pos maxValue)
        {
            if (value < minValue)
                return minValue;
            if (value > maxValue)
                return maxValue;
            return value;
        }

        inline Pos PosAbs(Pos value)
        {
            return value < 0 ? static_cast<Pos>(-value) : value;
        }

        FixedVec2 ToObstacleLocalFixed(const ObstacleWall& wall, Pos worldX, Pos worldY)
        {
            const int64_t dx = static_cast<int64_t>(worldX) - static_cast<int64_t>(wall.centerX);
            const int64_t dy = static_cast<int64_t>(worldY) - static_cast<int64_t>(wall.centerY);
            const int32_t cosQ = CosQ15(wall.rotation);
            const int32_t sinQ = SinQ15(wall.rotation);
            return {
                static_cast<Pos>((dx * cosQ + dy * sinQ) / kAngleSinCosScale),
                static_cast<Pos>((-dx * sinQ + dy * cosQ) / kAngleSinCosScale)
            };
        }

        void RotateWorldDirToLocal(int32_t worldDirXQ15, int32_t worldDirYQ15, Angle rotation, int32_t& localDirXQ15, int32_t& localDirYQ15)
        {
            const int32_t cosQ = CosQ15(rotation);
            const int32_t sinQ = SinQ15(rotation);
            localDirXQ15 = static_cast<int32_t>(
                (static_cast<int64_t>(worldDirXQ15) * cosQ + static_cast<int64_t>(worldDirYQ15) * sinQ)
                / kAngleSinCosScale);
            localDirYQ15 = static_cast<int32_t>(
                (-static_cast<int64_t>(worldDirXQ15) * sinQ + static_cast<int64_t>(worldDirYQ15) * cosQ)
                / kAngleSinCosScale);
        }

        FixedVec2 LocalDeltaToWorldFixed(const ObstacleWall& wall, Pos localX, Pos localY)
        {
            const int32_t cosQ = CosQ15(wall.rotation);
            const int32_t sinQ = SinQ15(wall.rotation);
            return {
                static_cast<Pos>((static_cast<int64_t>(localX) * cosQ - static_cast<int64_t>(localY) * sinQ) / kAngleSinCosScale),
                static_cast<Pos>((static_cast<int64_t>(localX) * sinQ + static_cast<int64_t>(localY) * cosQ) / kAngleSinCosScale)
            };
        }

        bool SlabAxis(
            int64_t origin,
            int32_t dir,
            int64_t minBound,
            int64_t maxBound,
            int64_t& tMin,
            int64_t& tMax)
        {
            if (dir == 0)
                return origin >= minBound && origin <= maxBound;

            int64_t t1 = ((minBound - origin) * kAngleSinCosScale) / dir;
            int64_t t2 = ((maxBound - origin) * kAngleSinCosScale) / dir;
            if (t1 > t2)
            {
                const int64_t tmp = t1;
                t1 = t2;
                t2 = tmp;
            }

            if (t1 > tMin)
                tMin = t1;
            if (t2 < tMax)
                tMax = t2;
            return tMin <= tMax;
        }

        Pos RaycastLocalAabb(
            Pos localOriginX,
            Pos localOriginY,
            int32_t localDirXQ15,
            int32_t localDirYQ15,
            Pos halfWidth,
            Pos halfHeight,
            Pos maxDistance)
        {
            if (localDirXQ15 == 0 && localDirYQ15 == 0)
                return maxDistance;

            int64_t tMin = 0;
            int64_t tMax = maxDistance;
            const int64_t minX = -static_cast<int64_t>(halfWidth);
            const int64_t maxX = static_cast<int64_t>(halfWidth);
            const int64_t minY = -static_cast<int64_t>(halfHeight);
            const int64_t maxY = static_cast<int64_t>(halfHeight);

            if (!SlabAxis(localOriginX, localDirXQ15, minX, maxX, tMin, tMax))
                return maxDistance;
            if (!SlabAxis(localOriginY, localDirYQ15, minY, maxY, tMin, tMax))
                return maxDistance;

            if (tMax < 0 || tMin > maxDistance)
                return maxDistance;

            int64_t hit = tMin >= 0 ? tMin : tMax;
            if (hit < 0 || hit > maxDistance)
                return maxDistance;
            return static_cast<Pos>(hit);
        }

        Pos RaycastObstacleFixed(
            Pos originX,
            Pos originY,
            int32_t dirXQ15,
            int32_t dirYQ15,
            Pos maxDistance,
            const ObstacleWall& wall)
        {
            const FixedVec2 localOrigin = ToObstacleLocalFixed(wall, originX, originY);
            int32_t localDirXQ15 = 0;
            int32_t localDirYQ15 = 0;
            RotateWorldDirToLocal(dirXQ15, dirYQ15, wall.rotation, localDirXQ15, localDirYQ15);
            return RaycastLocalAabb(
                localOrigin.x,
                localOrigin.y,
                localDirXQ15,
                localDirYQ15,
                wall.halfWidth,
                wall.halfHeight,
                maxDistance);
        }

        Pos RaycastObstaclesFixed(
            Pos originX,
            Pos originY,
            int32_t dirXQ15,
            int32_t dirYQ15,
            Pos maxDistance,
            const std::vector<ObstacleWall>& walls)
        {
            Pos best = maxDistance;
            for (const ObstacleWall& wall : walls)
            {
                const Pos hit = RaycastObstacleFixed(
                    originX, originY, dirXQ15, dirYQ15, maxDistance, wall);
                if (hit < best)
                    best = hit;
            }
            return best;
        }
    }

    ObstacleWall MakeObstacleWallFromPos(
        Pos centerX,
        Pos centerY,
        Pos halfWidth,
        Pos halfHeight,
        Angle rotation)
    {
        ObstacleWall wall;
        wall.centerX = centerX;
        wall.centerY = centerY;
        wall.halfWidth = halfWidth;
        wall.halfHeight = halfHeight;
        wall.rotation = rotation;
        return wall;
    }

    ObstacleWallState ToObstacleWallState(const ObstacleWall& wall)
    {
        ObstacleWallState state;
        state.centerX = PosToWorld(wall.centerX);
        state.centerY = PosToWorld(wall.centerY);
        state.width = PosToWorld(wall.halfWidth) * 2.0f;
        state.height = PosToWorld(wall.halfHeight) * 2.0f;
        state.rotation = AngleToRadians(wall.rotation);
        return state;
    }

    ObstacleWall InflateObstacleWall(const ObstacleWall& wall, Pos radius)
    {
        ObstacleWall inflated = wall;
        inflated.halfWidth = static_cast<Pos>(static_cast<int64_t>(inflated.halfWidth) + radius);
        inflated.halfHeight = static_cast<Pos>(static_cast<int64_t>(inflated.halfHeight) + radius);
        return inflated;
    }

    bool CircleIntersectsObstacleFixed(Pos posX, Pos posY, Pos radius, const ObstacleWall& wall)
    {
        const FixedVec2 local = ToObstacleLocalFixed(wall, posX, posY);
        const Pos clampX = ClampPos(local.x, static_cast<Pos>(-wall.halfWidth), wall.halfWidth);
        const Pos clampY = ClampPos(local.y, static_cast<Pos>(-wall.halfHeight), wall.halfHeight);
        const int64_t dx = static_cast<int64_t>(local.x) - static_cast<int64_t>(clampX);
        const int64_t dy = static_cast<int64_t>(local.y) - static_cast<int64_t>(clampY);
        const int64_t distSq = dx * dx + dy * dy;
        const int64_t radiusSq = static_cast<int64_t>(radius) * static_cast<int64_t>(radius);
        return distSq < radiusSq;
    }

    bool ResolveCircleObstacleCollisionFixed(FixedVec2& position, Pos radius, const ObstacleWall& wall)
    {
        const FixedVec2 local = ToObstacleLocalFixed(wall, position.x, position.y);
        const Pos clampX = ClampPos(local.x, static_cast<Pos>(-wall.halfWidth), wall.halfWidth);
        const Pos clampY = ClampPos(local.y, static_cast<Pos>(-wall.halfHeight), wall.halfHeight);
        const int64_t dx = static_cast<int64_t>(local.x) - static_cast<int64_t>(clampX);
        const int64_t dy = static_cast<int64_t>(local.y) - static_cast<int64_t>(clampY);
        const int64_t distSq = dx * dx + dy * dy;
        const int64_t radiusSq = static_cast<int64_t>(radius) * static_cast<int64_t>(radius);
        if (distSq >= radiusSq)
            return false;

        Pos push = 0;
        Pos localPushX = 0;
        Pos localPushY = 0;
        if (distSq > 0)
        {
            const int32_t dist = Isqrt64(distSq);
            if (dist <= 0)
                return false;

            push = static_cast<Pos>(static_cast<int64_t>(radius) - dist);
            const int32_t normalXQ15 = static_cast<int32_t>((dx * kAngleSinCosScale) / dist);
            const int32_t normalYQ15 = static_cast<int32_t>((dy * kAngleSinCosScale) / dist);
            localPushX = static_cast<Pos>((static_cast<int64_t>(normalXQ15) * push) / kAngleSinCosScale);
            localPushY = static_cast<Pos>((static_cast<int64_t>(normalYQ15) * push) / kAngleSinCosScale);
        }
        else
        {
            const Pos penX = static_cast<Pos>(static_cast<int64_t>(wall.halfWidth) - PosAbs(local.x));
            const Pos penY = static_cast<Pos>(static_cast<int64_t>(wall.halfHeight) - PosAbs(local.y));
            if (penX < penY)
            {
                localPushX = local.x >= 0 ? static_cast<Pos>(static_cast<int64_t>(radius) + penX)
                                          : static_cast<Pos>(-static_cast<int64_t>(radius) - penX);
            }
            else
            {
                localPushY = local.y >= 0 ? static_cast<Pos>(static_cast<int64_t>(radius) + penY)
                                          : static_cast<Pos>(-static_cast<int64_t>(radius) - penY);
            }
        }

        const FixedVec2 worldPush = LocalDeltaToWorldFixed(wall, localPushX, localPushY);
        position.x = static_cast<Pos>(static_cast<int64_t>(position.x) + worldPush.x);
        position.y = static_cast<Pos>(static_cast<int64_t>(position.y) + worldPush.y);
        return true;
    }

    Pos RaycastObstaclesFromAngle(
        Pos originX,
        Pos originY,
        Angle direction,
        Pos maxDistance,
        const std::vector<ObstacleWall>& walls)
    {
        return RaycastObstaclesFixed(
            originX,
            originY,
            CosQ15(direction),
            SinQ15(direction),
            maxDistance,
            walls);
    }

    bool HasLineOfSightFixed(
        Pos fromX,
        Pos fromY,
        Pos toX,
        Pos toY,
        const std::vector<ObstacleWall>& walls,
        Pos clearancePos)
    {
        const int64_t dx = static_cast<int64_t>(toX) - static_cast<int64_t>(fromX);
        const int64_t dy = static_cast<int64_t>(toY) - static_cast<int64_t>(fromY);
        const int64_t distSq = dx * dx + dy * dy;
        if (distSq <= 1)
            return true;

        const int32_t dist = Isqrt64(distSq);
        if (dist <= 0)
            return true;

        const Pos maxDist = static_cast<Pos>(dist - clearancePos);
        if (maxDist <= 0)
            return true;

        const int32_t dirXQ15 = static_cast<int32_t>((dx * kAngleSinCosScale) / dist);
        const int32_t dirYQ15 = static_cast<int32_t>((dy * kAngleSinCosScale) / dist);
        const Pos hit = RaycastObstaclesFixed(fromX, fromY, dirXQ15, dirYQ15, maxDist, walls);
        const Pos tolerancePos = static_cast<Pos>(kAiObstacleClearancePosValue);
        return hit >= maxDist - tolerancePos;
    }

    bool PointInsideObstacleFixed(Pos posX, Pos posY, const ObstacleWall& wall)
    {
        const FixedVec2 local = ToObstacleLocalFixed(wall, posX, posY);
        return PosAbs(local.x) <= wall.halfWidth && PosAbs(local.y) <= wall.halfHeight;
    }
}

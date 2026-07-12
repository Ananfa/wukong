#pragma once
#include "../Common/AngleLUT.h"
#include "../Common/FixedMath.h"
#include "../Common/Types.h"
#include <vector>

namespace TankBattle
{
    struct ObstacleWall
    {
        Pos centerX = 0;
        Pos centerY = 0;
        Pos halfWidth = 0;
        Pos halfHeight = 0;
        Angle rotation = 0;
    };

    ObstacleWall MakeObstacleWallFromPos(
        Pos centerX,
        Pos centerY,
        Pos halfWidth,
        Pos halfHeight,
        Angle rotation);

    ObstacleWallState ToObstacleWallState(const ObstacleWall& wall);

    bool CircleIntersectsObstacleFixed(Pos posX, Pos posY, Pos radius, const ObstacleWall& wall);

    bool ResolveCircleObstacleCollisionFixed(FixedVec2& position, Pos radius, const ObstacleWall& wall);

    Pos RaycastObstaclesFromAngle(
        Pos originX,
        Pos originY,
        Angle direction,
        Pos maxDistance,
        const std::vector<ObstacleWall>& walls);

    bool HasLineOfSightFixed(
        Pos fromX,
        Pos fromY,
        Pos toX,
        Pos toY,
        const std::vector<ObstacleWall>& walls,
        Pos clearancePos = 0);

    bool PointInsideObstacleFixed(Pos posX, Pos posY, const ObstacleWall& wall);

    ObstacleWall InflateObstacleWall(const ObstacleWall& wall, Pos radius);
}

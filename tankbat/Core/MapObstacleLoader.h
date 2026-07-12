#pragma once
#include "ObstacleWall.h"
#include "../Common/Types.h"
#include <string>
#include <vector>

namespace TankBattle
{
    // JSON 整数均为 Pos 子单位（1 世界单位 = kRngWorldSubunitsPerUnit）
    // 墙 rotationAngle 为 0..65535 离散角度
    bool ParseMapConfigFromJson(
        const std::string& json,
        std::vector<ObstacleWall>& outWalls,
        Pos& outWorldWidthPos,
        Pos& outWorldHeightPos,
        FactionSpawnZone outSpawnZones[kFactionCount],
        bool& outHasSpawnZones);

    void LoadDefaultMapObstacles(
        std::vector<ObstacleWall>& outWalls,
        Pos& outWorldWidthPos,
        Pos& outWorldHeightPos);

    void LoadDefaultSpawnZones(FactionSpawnZone outSpawnZones[kFactionCount]);
}

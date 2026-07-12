#pragma once
#include "../Common/AngleLUT.h"
#include "../Common/FixedMath.h"
#include "../Common/Types.h"
#include <cstdint>

namespace TankBattle
{
    // AI / 逻辑层只读视图：定点坐标，不经 float 快照
    struct TankLogicView
    {
        uint32_t id = 0;
        uint32_t playerId = 0;
        Faction faction = Faction::Soviet;
        TankType type = TankType::T34;
        FixedVec2 position;
        Angle rotation = 0;
        int32_t hp = 0;
        int32_t maxHp = 0;
        bool isAlive = false;
        Pos sizePos = 0;
    };
}

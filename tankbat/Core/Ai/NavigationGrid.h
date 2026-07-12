#pragma once
#include "../Common/FixedMath.h"
#include "../Common/Types.h"
#include "../ObstacleWall.h"
#include <cstdint>
#include <vector>

namespace TankBattle
{
    class NavigationGrid
    {
    public:
        void Build(
            Pos worldWidthPos,
            Pos worldHeightPos,
            Pos cellSizePos,
            const std::vector<ObstacleWall>& walls,
            Pos agentRadiusPos);

        bool IsValid() const { return m_cols > 0 && m_rows > 0 && !m_blocked.empty(); }

        bool IsWalkable(int cellX, int cellY) const;
        bool IsInside(int cellX, int cellY) const;

        void WorldToCell(Pos worldX, Pos worldY, int& cellX, int& cellY) const;
        void WorldToCell(const FixedVec2& worldPos, int& cellX, int& cellY) const;
        FixedVec2 CellCenterToFixed(int cellX, int cellY) const;

        bool FindNearestWalkable(int cellX, int cellY, int& outCellX, int& outCellY) const;
        bool PickRandomWalkableCell(int& outCellX, int& outCellY, uint32_t seed) const;

        bool FindPath(
            const FixedVec2& startFixed,
            const FixedVec2& goalFixed,
            std::vector<FixedVec2>& outWaypoints) const;

    private:
        int CellIndex(int cellX, int cellY) const;
        bool IsCellBlocked(int cellX, int cellY, const std::vector<ObstacleWall>& inflatedWalls) const;

        int m_cols = 0;
        int m_rows = 0;
        Pos m_cellSizePos = static_cast<Pos>(kNavGridCellSizePosValue);
        Pos m_worldWidthPos = static_cast<Pos>(kDefaultWorldSizePosValue);
        Pos m_worldHeightPos = static_cast<Pos>(kDefaultWorldSizePosValue);
        Pos m_agentRadiusPos = 0;
        std::vector<uint8_t> m_blocked;
    };
}

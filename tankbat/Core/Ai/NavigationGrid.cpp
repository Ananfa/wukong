#include "NavigationGrid.h"
#include "../Common/Constants.h"
#include "../Common/FixedMath.h"
#include <queue>
#include <vector>

namespace TankBattle
{
    namespace
    {
        struct AStarOpenNode
        {
            int cellX = 0;
            int cellY = 0;
            int32_t fScore = 0;

            bool operator>(const AStarOpenNode& other) const
            {
                if (fScore != other.fScore)
                    return fScore > other.fScore;
                if (cellY != other.cellY)
                    return cellY > other.cellY;
                return cellX > other.cellX;
            }
        };

        int32_t OctileHeuristic(int dx, int dy)
        {
            if (dx < 0)
                dx = -dx;
            if (dy < 0)
                dy = -dy;
            const int minAxis = dx < dy ? dx : dy;
            const int maxAxis = dx > dy ? dx : dy;
            return maxAxis * kNavGridStraightCost + minAxis * kNavGridHeuristicDiagFactor;
        }

        ObstacleWall InflateWall(const ObstacleWall& wall, Pos radiusPos)
        {
            return InflateObstacleWall(wall, radiusPos);
        }
    }

    void NavigationGrid::Build(
        Pos worldWidthPos,
        Pos worldHeightPos,
        Pos cellSizePos,
        const std::vector<ObstacleWall>& walls,
        Pos agentRadiusPos)
    {
        m_worldWidthPos = worldWidthPos;
        m_worldHeightPos = worldHeightPos;
        m_cellSizePos = cellSizePos > 0 ? cellSizePos : static_cast<Pos>(kNavGridCellSizePosValue);
        m_agentRadiusPos = agentRadiusPos;
        m_cols = static_cast<int>((worldWidthPos + m_cellSizePos - 1) / m_cellSizePos);
        m_rows = static_cast<int>((worldHeightPos + m_cellSizePos - 1) / m_cellSizePos);
        if (m_cols < 1) m_cols = 1;
        if (m_rows < 1) m_rows = 1;

        const Pos inflateRadiusPos = static_cast<Pos>(kNavGridWallInflateRadiusPosValue);
        std::vector<ObstacleWall> inflatedWalls;
        inflatedWalls.reserve(walls.size());
        for (const ObstacleWall& wall : walls)
            inflatedWalls.push_back(InflateWall(wall, inflateRadiusPos));

        m_blocked.assign(static_cast<size_t>(m_cols * m_rows), 0);
        for (int y = 0; y < m_rows; ++y)
        {
            for (int x = 0; x < m_cols; ++x)
            {
                if (IsCellBlocked(x, y, inflatedWalls))
                    m_blocked[CellIndex(x, y)] = 1;
            }
        }
    }

    bool NavigationGrid::IsWalkable(int cellX, int cellY) const
    {
        if (!IsInside(cellX, cellY))
            return false;
        return m_blocked[CellIndex(cellX, cellY)] == 0;
    }

    bool NavigationGrid::IsInside(int cellX, int cellY) const
    {
        return cellX >= 0 && cellY >= 0 && cellX < m_cols && cellY < m_rows;
    }

    void NavigationGrid::WorldToCell(Pos worldX, Pos worldY, int& cellX, int& cellY) const
    {
        cellX = worldX / m_cellSizePos;
        cellY = worldY / m_cellSizePos;
        if (cellX < 0) cellX = 0;
        if (cellY < 0) cellY = 0;
        if (cellX >= m_cols) cellX = m_cols - 1;
        if (cellY >= m_rows) cellY = m_rows - 1;
    }

    void NavigationGrid::WorldToCell(const FixedVec2& worldPos, int& cellX, int& cellY) const
    {
        WorldToCell(worldPos.x, worldPos.y, cellX, cellY);
    }

    FixedVec2 NavigationGrid::CellCenterToFixed(int cellX, int cellY) const
    {
        return {
            static_cast<Pos>((static_cast<int64_t>(cellX) * 2 + 1) * m_cellSizePos / 2),
            static_cast<Pos>((static_cast<int64_t>(cellY) * 2 + 1) * m_cellSizePos / 2)
        };
    }

    int NavigationGrid::CellIndex(int cellX, int cellY) const
    {
        return cellY * m_cols + cellX;
    }

    bool NavigationGrid::IsCellBlocked(
        int cellX,
        int cellY,
        const std::vector<ObstacleWall>& inflatedWalls) const
    {
        const FixedVec2 center = CellCenterToFixed(cellX, cellY);
        if (center.x - m_agentRadiusPos < 0 ||
            center.y - m_agentRadiusPos < 0 ||
            center.x + m_agentRadiusPos > m_worldWidthPos ||
            center.y + m_agentRadiusPos > m_worldHeightPos)
        {
            return true;
        }

        const Pos sampleRadiusPos = m_cellSizePos / 2;
        for (const ObstacleWall& wall : inflatedWalls)
        {
            if (CircleIntersectsObstacleFixed(center.x, center.y, sampleRadiusPos, wall))
                return true;
        }
        return false;
    }

    bool NavigationGrid::FindNearestWalkable(
        int cellX,
        int cellY,
        int& outCellX,
        int& outCellY) const
    {
        if (IsWalkable(cellX, cellY))
        {
            outCellX = cellX;
            outCellY = cellY;
            return true;
        }

        const int maxRadius = 16;
        for (int radius = 1; radius <= maxRadius; ++radius)
        {
            for (int dy = -radius; dy <= radius; ++dy)
            {
                for (int dx = -radius; dx <= radius; ++dx)
                {
                    if (dx * dx + dy * dy > radius * radius)
                        continue;
                    int nx = cellX + dx;
                    int ny = cellY + dy;
                    if (!IsWalkable(nx, ny))
                        continue;
                    outCellX = nx;
                    outCellY = ny;
                    return true;
                }
            }
        }
        return false;
    }

    bool NavigationGrid::PickRandomWalkableCell(
        int& outCellX,
        int& outCellY,
        uint32_t seed) const
    {
        if (m_blocked.empty())
            return false;

        size_t walkableCount = 0;
        for (uint8_t blocked : m_blocked)
        {
            if (blocked == 0)
                ++walkableCount;
        }
        if (walkableCount == 0)
            return false;

        uint32_t pick = seed % static_cast<uint32_t>(walkableCount);
        for (int y = 0; y < m_rows; ++y)
        {
            for (int x = 0; x < m_cols; ++x)
            {
                if (!IsWalkable(x, y))
                    continue;
                if (pick == 0)
                {
                    outCellX = x;
                    outCellY = y;
                    return true;
                }
                --pick;
            }
        }
        return false;
    }

    bool NavigationGrid::FindPath(
        const FixedVec2& startFixed,
        const FixedVec2& goalFixed,
        std::vector<FixedVec2>& outWaypoints) const
    {
        outWaypoints.clear();
        if (!IsValid())
            return false;

        int startX = 0;
        int startY = 0;
        int goalX = 0;
        int goalY = 0;
        WorldToCell(startFixed, startX, startY);
        WorldToCell(goalFixed, goalX, goalY);

        if (!FindNearestWalkable(startX, startY, startX, startY))
            return false;
        if (!FindNearestWalkable(goalX, goalY, goalX, goalY))
            return false;

        if (startX == goalX && startY == goalY)
        {
            outWaypoints.push_back(CellCenterToFixed(goalX, goalY));
            return true;
        }

        const size_t cellCount = static_cast<size_t>(m_cols * m_rows);
        std::vector<int32_t> gScore(cellCount, kNavGridCostInf);
        std::vector<int> parentX(cellCount, -1);
        std::vector<int> parentY(cellCount, -1);
        std::vector<uint8_t> closed(cellCount, 0);

        std::priority_queue<AStarOpenNode, std::vector<AStarOpenNode>, std::greater<AStarOpenNode>> open;

        auto pushOpen = [&](int cx, int cy, int32_t g, int fromX, int fromY) {
            int dx = goalX - cx;
            int dy = goalY - cy;
            if (dx < 0) dx = -dx;
            if (dy < 0) dy = -dy;
            AStarOpenNode node;
            node.cellX = cx;
            node.cellY = cy;
            node.fScore = g + OctileHeuristic(dx, dy);
            open.push(node);
            parentX[CellIndex(cx, cy)] = fromX;
            parentY[CellIndex(cx, cy)] = fromY;
            gScore[CellIndex(cx, cy)] = g;
        };

        pushOpen(startX, startY, 0, -1, -1);

        static const int kNeighborCount = 8;
        static const int kNeighborDx[kNeighborCount] = {1, -1, 0, 0, 1, 1, -1, -1};
        static const int kNeighborDy[kNeighborCount] = {0, 0, 1, -1, 1, -1, 1, -1};
        static const int32_t kNeighborCost[kNeighborCount] = {
            kNavGridStraightCost,
            kNavGridStraightCost,
            kNavGridStraightCost,
            kNavGridStraightCost,
            kNavGridDiagonalCost,
            kNavGridDiagonalCost,
            kNavGridDiagonalCost,
            kNavGridDiagonalCost
        };

        bool found = false;
        while (!open.empty())
        {
            AStarOpenNode current = open.top();
            open.pop();

            if (closed[CellIndex(current.cellX, current.cellY)])
                continue;

            const int currentIdx = CellIndex(current.cellX, current.cellY);
            int hdx = goalX - current.cellX;
            int hdy = goalY - current.cellY;
            if (hdx < 0) hdx = -hdx;
            if (hdy < 0) hdy = -hdy;
            if (current.fScore > gScore[currentIdx] + OctileHeuristic(hdx, hdy))
                continue;

            closed[currentIdx] = 1;

            if (current.cellX == goalX && current.cellY == goalY)
            {
                found = true;
                break;
            }

            for (int i = 0; i < kNeighborCount; ++i)
            {
                int nx = current.cellX + kNeighborDx[i];
                int ny = current.cellY + kNeighborDy[i];
                if (!IsWalkable(nx, ny))
                    continue;

                if (i >= 4)
                {
                    if (!IsWalkable(current.cellX + kNeighborDx[i], current.cellY) ||
                        !IsWalkable(current.cellX, current.cellY + kNeighborDy[i]))
                    {
                        continue;
                    }
                }

                const int32_t tentativeG =
                    gScore[CellIndex(current.cellX, current.cellY)] + kNeighborCost[i];
                const int idx = CellIndex(nx, ny);
                if (tentativeG >= gScore[idx])
                    continue;

                gScore[idx] = tentativeG;
                pushOpen(nx, ny, tentativeG, current.cellX, current.cellY);
            }
        }

        if (!found)
            return false;

        std::vector<FixedVec2> reversed;
        int cx = goalX;
        int cy = goalY;
        while (cx >= 0 && cy >= 0)
        {
            reversed.push_back(CellCenterToFixed(cx, cy));
            int px = parentX[CellIndex(cx, cy)];
            int py = parentY[CellIndex(cx, cy)];
            if (px < 0 || py < 0)
                break;
            cx = px;
            cy = py;
        }

        outWaypoints.assign(reversed.rbegin(), reversed.rend());
        if (!outWaypoints.empty())
        {
            const Pos halfCellPos = m_cellSizePos / 2;
            const int64_t halfCellSq =
                static_cast<int64_t>(halfCellPos) * static_cast<int64_t>(halfCellPos);
            if (FixedDistanceSquared(outWaypoints.back(), goalFixed) > halfCellSq)
                outWaypoints.push_back(goalFixed);
        }
        return !outWaypoints.empty();
    }
}

#include "AiController.h"
#include "AiPerception.h"
#include "AiSteering.h"
#include "../ObstacleWall.h"
#include "Constants.h"

namespace TankBattle
{
    AiController::AiController()
        : m_root(BuildDefaultTankBehaviorTree())
    {
    }

    AiTankMemory& AiController::GetOrCreateMemory(uint32_t tankId, uint32_t frameIndex, const GameRng& rng)
    {
        auto it = m_memories.find(tankId);
        if (it == m_memories.end())
        {
            // 初始朝向只用 seed+tankId，不用创建时的 frame，避免 ClearAi 后各方在不同帧重建记忆时分叉
            (void)frameIndex;
            AiTankMemory memory;
            memory.wanderHeading = static_cast<Angle>(rng.UniformAngleUnits(
                0,
                RngPurpose::AiInitialHeading,
                tankId));
            it = m_memories.emplace(tankId, memory).first;
        }
        return it->second;
    }

    AiIntent AiController::Tick(
        Tank& self,
        const std::vector<std::shared_ptr<Tank>>& allTanks,
        const std::vector<ObstacleWall>& obstacles,
        const NavigationGrid& navGrid,
        uint32_t frameIndex,
        const GameRng& rng)
    {
        AiIntent intent;
        AiBlackboard bb;
        AiTankMemory& memory = GetOrCreateMemory(self.GetId(), frameIndex, rng);

        AiPerception::Refresh(self, allTanks, obstacles, bb, memory);

        BTContext ctx;
        ctx.self = &self;
        ctx.bb = &bb;
        ctx.intent = &intent;
        ctx.memory = &memory;
        ctx.allTanks = &allTanks;
        ctx.navGrid = navGrid.IsValid() ? &navGrid : nullptr;
        ctx.frameIndex = frameIndex;
        ctx.gameRng = &rng;

        if (m_root)
            m_root->Tick(ctx);

        AiSteering::ApplyObstacleAvoidance(intent, self, obstacles);

        return intent;
    }

    void AiController::Clear()
    {
        m_memories.clear();
    }

    void AiController::RemoveMemory(uint32_t tankId)
    {
        m_memories.erase(tankId);
    }

    void AiController::ExportMemories(std::vector<AiTankMemorySnapshot>& out) const
    {
        out.clear();
        out.reserve(m_memories.size());
        for (const auto& entry : m_memories)
        {
            const uint32_t tankId = entry.first;
            const AiTankMemory& mem = entry.second;
            AiTankMemorySnapshot snap;
            snap.tankId = tankId;
            snap.wanderHeading = static_cast<int32_t>(mem.wanderHeading);
            snap.strafeSign = mem.strafeSign;
            snap.strafeSwitchFrames = mem.strafeSwitchFramesRemaining;
            snap.wanderGoalSerial = mem.wanderGoalSerial;
            snap.pathGoalX = mem.pathGoal.x;
            snap.pathGoalY = mem.pathGoal.y;
            snap.pathTargetId = mem.pathTargetId;
            snap.pathMoveMode = static_cast<uint32_t>(mem.pathMoveMode);
            snap.pathRecalcFrames = mem.pathRecalcFramesRemaining;
            snap.wanderPathGoalX = mem.wanderPathGoal.x;
            snap.wanderPathGoalY = mem.wanderPathGoal.y;
            snap.wanderPathFrames = mem.wanderPathFramesRemaining;
            snap.pathWaypointIndex = static_cast<uint32_t>(mem.pathWaypointIndex);

            const size_t n = mem.pathWaypoints.size() < kAiMemoryMaxWaypoints
                ? mem.pathWaypoints.size()
                : kAiMemoryMaxWaypoints;
            snap.pathWaypointCoords.reserve(n * 2);
            for (size_t i = 0; i < n; ++i)
            {
                snap.pathWaypointCoords.push_back(mem.pathWaypoints[i].x);
                snap.pathWaypointCoords.push_back(mem.pathWaypoints[i].y);
            }
            if (snap.pathWaypointIndex >= n && n > 0)
                snap.pathWaypointIndex = static_cast<uint32_t>(n - 1);
            out.push_back(std::move(snap));
        }
    }

    void AiController::ApplyMemories(const std::vector<AiTankMemorySnapshot>& memories)
    {
        m_memories.clear();
        for (const auto& snap : memories)
        {
            if (snap.tankId == 0)
                continue;
            AiTankMemory mem;
            mem.wanderHeading = static_cast<Angle>(snap.wanderHeading);
            mem.strafeSign = snap.strafeSign == 0 ? 1 : snap.strafeSign;
            mem.strafeSwitchFramesRemaining = snap.strafeSwitchFrames;
            mem.wanderGoalSerial = snap.wanderGoalSerial;
            mem.pathGoal.x = snap.pathGoalX;
            mem.pathGoal.y = snap.pathGoalY;
            mem.pathTargetId = snap.pathTargetId;
            mem.pathMoveMode = static_cast<AiMoveMode>(snap.pathMoveMode);
            mem.pathRecalcFramesRemaining = snap.pathRecalcFrames;
            mem.wanderPathGoal.x = snap.wanderPathGoalX;
            mem.wanderPathGoal.y = snap.wanderPathGoalY;
            mem.wanderPathFramesRemaining = snap.wanderPathFrames;

            const size_t pairCount = snap.pathWaypointCoords.size() / 2;
            const size_t n = pairCount < kAiMemoryMaxWaypoints ? pairCount : kAiMemoryMaxWaypoints;
            mem.pathWaypoints.reserve(n);
            for (size_t i = 0; i < n; ++i)
            {
                FixedVec2 wp;
                wp.x = snap.pathWaypointCoords[i * 2];
                wp.y = snap.pathWaypointCoords[i * 2 + 1];
                mem.pathWaypoints.push_back(wp);
            }
            mem.pathWaypointIndex = snap.pathWaypointIndex;
            if (mem.pathWaypointIndex >= mem.pathWaypoints.size() && !mem.pathWaypoints.empty())
                mem.pathWaypointIndex = mem.pathWaypoints.size() - 1;
            m_memories[snap.tankId] = std::move(mem);
        }
    }
}

#include "AiPathFollower.h"
#include "../Common/Constants.h"
#include "../Common/AngleLUT.h"
#include "../Common/FixedMath.h"
#include "../TankLogicView.h"

namespace TankBattle
{
    namespace
    {
        Angle HeadingTowardFixed(const FixedVec2& fromFixed, const FixedVec2& toFixed)
        {
            const int32_t dx = toFixed.x - fromFixed.x;
            const int32_t dy = toFixed.y - fromFixed.y;
            return Atan2Pos(dy, dx);
        }

        void ApplyDirectMove(BTContext& ctx, const FixedVec2& goalFixed, AiMoveMode mode, int32_t speedScaleQ15)
        {
            const TankLogicView selfView = ctx.self->GetLogicView();
            const Angle heading = HeadingTowardFixed(selfView.position, goalFixed);
            ApplyMoveHeading(*ctx.intent, *ctx.bb, mode, heading, speedScaleQ15);
        }

        void ClearPathMemory(AiTankMemory& memory)
        {
            memory.pathWaypoints.clear();
            memory.pathWaypointIndex = 0;
            memory.pathTargetId = 0;
            memory.pathMoveMode = AiMoveMode::None;
        }
    }

    bool AiPathFollower::MoveToward(
        BTContext& ctx,
        const NavigationGrid* grid,
        const FixedVec2& goalFixed,
        AiMoveMode mode,
        int32_t speedScaleQ15,
        uint32_t relatedTargetId)
    {
        if (!ctx.self || !ctx.intent || !ctx.bb || !ctx.memory)
            return false;

        if (!grid || !grid->IsValid())
        {
            ClearPathMemory(*ctx.memory);
            ApplyDirectMove(ctx, goalFixed, mode, speedScaleQ15);
            return true;
        }

        const TankLogicView selfView = ctx.self->GetLogicView();
        if (ctx.memory->pathRecalcFramesRemaining > 0)
            --ctx.memory->pathRecalcFramesRemaining;

        const int64_t goalShiftSq = FixedDistanceSquared(ctx.memory->pathGoal, goalFixed);
        const int64_t goalShiftThresholdSq =
            PosDistanceSquared(static_cast<Pos>(kAiPathGoalMoveThresholdPosValue));
        bool atPathEnd = !ctx.memory->pathWaypoints.empty() &&
            ctx.memory->pathWaypointIndex + 1 >= ctx.memory->pathWaypoints.size();
        const int64_t distToGoalSq = FixedDistanceSquared(selfView.position, goalFixed);
        const int64_t waypointReachExtendedSq =
            (PosDistanceSquared(static_cast<Pos>(kAiPathWaypointReachPosValue)) * 9) / 4;
        bool needRecalc = ctx.memory->pathWaypoints.empty() ||
            ctx.memory->pathMoveMode != mode ||
            ctx.memory->pathTargetId != relatedTargetId ||
            goalShiftSq > goalShiftThresholdSq ||
            ctx.memory->pathRecalcFramesRemaining <= 0 ||
            (atPathEnd && distToGoalSq > waypointReachExtendedSq);

        if (needRecalc)
        {
            ctx.memory->pathRecalcFramesRemaining = kAiPathRecalcFrames;
            ctx.memory->pathGoal = goalFixed;
            ctx.memory->pathMoveMode = mode;
            ctx.memory->pathTargetId = relatedTargetId;
            ctx.memory->pathWaypointIndex = 0;

            if (!grid->FindPath(selfView.position, goalFixed, ctx.memory->pathWaypoints))
            {
                ClearPathMemory(*ctx.memory);
                if (mode == AiMoveMode::WanderNoTarget)
                    ApplyDirectMove(ctx, goalFixed, mode, speedScaleQ15);
                return mode == AiMoveMode::WanderNoTarget;
            }
        }

        if (ctx.memory->pathWaypoints.empty())
        {
            ApplyDirectMove(ctx, goalFixed, mode, speedScaleQ15);
            return true;
        }

        const int64_t reachSq =
            PosDistanceSquared(static_cast<Pos>(kAiPathWaypointReachPosValue));
        while (ctx.memory->pathWaypointIndex + 1 < ctx.memory->pathWaypoints.size())
        {
            const FixedVec2& waypoint = ctx.memory->pathWaypoints[ctx.memory->pathWaypointIndex];
            if (FixedDistanceSquared(selfView.position, waypoint) > reachSq)
                break;
            ++ctx.memory->pathWaypointIndex;
        }

        FixedVec2 moveGoal = ctx.memory->pathWaypoints[ctx.memory->pathWaypointIndex];
        if (FixedDistanceSquared(selfView.position, moveGoal) <= reachSq &&
            ctx.memory->pathWaypointIndex + 1 < ctx.memory->pathWaypoints.size())
        {
            moveGoal = ctx.memory->pathWaypoints[++ctx.memory->pathWaypointIndex];
        }

        const Angle heading = HeadingTowardFixed(selfView.position, moveGoal);
        AiMoveMode outMode = ctx.memory->pathWaypoints.size() > 1
            ? AiMoveMode::FollowPath
            : mode;
        ApplyMoveHeading(*ctx.intent, *ctx.bb, outMode, heading, speedScaleQ15);
        return true;
    }

    bool AiPathFollower::WanderOnGrid(BTContext& ctx, const NavigationGrid* grid, int32_t speedScaleQ15)
    {
        if (!ctx.self || !ctx.intent || !ctx.bb || !ctx.memory)
            return false;

        if (!grid || !grid->IsValid())
        {
            if (ctx.gameRng &&
                ctx.gameRng->UniformInt(
                    ctx.frameIndex,
                    RngPurpose::AiWanderTurn,
                    ctx.self->GetId(),
                    0,
                    100,
                    0) == 0)
            {
                ctx.memory->wanderHeading = static_cast<Angle>(ctx.gameRng->UniformAngleUnits(
                    ctx.frameIndex,
                    RngPurpose::AiInitialHeading,
                    ctx.self->GetId(),
                    ++ctx.memory->wanderGoalSerial));
            }

            ctx.intent->targetId = 0;
            ApplyMoveHeading(
                *ctx.intent,
                *ctx.bb,
                AiMoveMode::WanderNoTarget,
                ctx.memory->wanderHeading,
                speedScaleQ15);
            return true;
        }

        if (ctx.memory->wanderPathFramesRemaining > 0)
            --ctx.memory->wanderPathFramesRemaining;

        const TankLogicView selfView = ctx.self->GetLogicView();
        const int64_t reachSq =
            PosDistanceSquared(static_cast<Pos>(kAiPathWaypointReachPosValue));
        bool needNewGoal = ctx.memory->wanderPathFramesRemaining <= 0 ||
            FixedDistanceSquared(selfView.position, ctx.memory->wanderPathGoal) < reachSq;

        if (needNewGoal)
        {
            ctx.memory->wanderPathFramesRemaining = kAiWanderGoalFrames;
            int cellX = 0;
            int cellY = 0;
            if (ctx.gameRng &&
                grid->PickRandomWalkableCell(
                    cellX,
                    cellY,
                    ctx.gameRng->SubSeed(
                        ctx.frameIndex,
                        RngPurpose::AiWanderGoal,
                        ctx.self->GetId(),
                        ++ctx.memory->wanderGoalSerial)))
            {
                ctx.memory->wanderPathGoal = grid->CellCenterToFixed(cellX, cellY);
            }
            else
                ctx.memory->wanderPathGoal = selfView.position;
        }

        ctx.intent->targetId = 0;
        return MoveToward(
            ctx,
            grid,
            ctx.memory->wanderPathGoal,
            AiMoveMode::WanderNoTarget,
            speedScaleQ15,
            0);
    }
}

#include "TankBehaviorTree.h"
#include "../AiMovement.h"
#include "../AiPathFollower.h"
#include "../AiPerception.h"
#include "../../Common/AngleLUT.h"
#include "../../Common/Constants.h"
#include "../../Common/FixedMath.h"
#include "../../TankLogicView.h"

namespace TankBattle
{
    namespace
    {
        BTStatus ActSelectTarget(BTContext& ctx)
        {
            if (!ctx.bb->hasTarget)
                return BTStatus::Failure;

            ctx.intent->targetId = ctx.bb->targetId;
            return BTStatus::Success;
        }

        BTStatus ActFireIfReady(BTContext& ctx)
        {
            if (ctx.bb->hasTarget && ctx.bb->hasLineOfSight && ctx.bb->canFire)
                ctx.intent->wantFire = true;
            return BTStatus::Success;
        }

        BTStatus ActUseAbility(BTContext& ctx)
        {
            if (!ctx.bb->abilityReady)
                return BTStatus::Failure;

            ctx.intent->wantAbility = true;
            return BTStatus::Success;
        }

        FixedVec2 OffsetFromFixedPos(
            Pos fromX,
            Pos fromY,
            int32_t dirXQ15,
            int32_t dirYQ15,
            Pos offsetPos)
        {
            return {
                static_cast<Pos>(
                    static_cast<int64_t>(fromX)
                    + (static_cast<int64_t>(dirXQ15) * offsetPos) / kAngleSinCosScale),
                static_cast<Pos>(
                    static_cast<int64_t>(fromY)
                    + (static_cast<int64_t>(dirYQ15) * offsetPos) / kAngleSinCosScale)
            };
        }

        bool NormalizeDeltaQ15(int64_t dx, int64_t dy, int32_t& outDirXQ15, int32_t& outDirYQ15)
        {
            const int64_t distSq = dx * dx + dy * dy;
            if (distSq <= 1)
                return false;

            const int32_t dist = Isqrt64(distSq);
            if (dist <= 0)
                return false;

            outDirXQ15 = static_cast<int32_t>((dx * kAngleSinCosScale) / dist);
            outDirYQ15 = static_cast<int32_t>((dy * kAngleSinCosScale) / dist);
            return true;
        }

        FixedVec2 ComputeApproachGoal(
            const FixedVec2& selfPos,
            const FixedVec2& targetPos,
            Pos engageRangePos)
        {
            const int64_t dx = static_cast<int64_t>(targetPos.x) - static_cast<int64_t>(selfPos.x);
            const int64_t dy = static_cast<int64_t>(targetPos.y) - static_cast<int64_t>(selfPos.y);
            const int64_t distSq = dx * dx + dy * dy;
            const int64_t engageSq = PosDistanceSquared(engageRangePos);
            if (distSq <= engageSq)
                return targetPos;

            const int32_t dist = Isqrt64(distSq);
            if (dist <= 0)
                return targetPos;

            const int64_t stopDist =
                static_cast<int64_t>(dist)
                - (static_cast<int64_t>(engageRangePos) * 85 / 100);
            if (stopDist <= 0)
                return selfPos;

            return {
                static_cast<Pos>(static_cast<int64_t>(selfPos.x) + (dx * stopDist) / dist),
                static_cast<Pos>(static_cast<int64_t>(selfPos.y) + (dy * stopDist) / dist)
            };
        }

        BTStatus ActApproach(BTContext& ctx)
        {
            if (!ctx.bb->hasTarget || !ctx.self) return BTStatus::Failure;

            const TankLogicView selfView = ctx.self->GetLogicView();
            for (const auto& other : *ctx.allTanks)
            {
                if (!other || other->GetId() != ctx.bb->targetId) continue;
                const TankLogicView targetView = other->GetLogicView();
                FixedVec2 goal = ComputeApproachGoal(
                    selfView.position,
                    targetView.position,
                    ctx.bb->engageRangePos);
                AiPathFollower::MoveToward(
                    ctx,
                    ctx.navGrid,
                    goal,
                    AiMoveMode::ApproachTarget,
                    kAiSpeedScaleFullQ15,
                    ctx.bb->targetId);
                return BTStatus::Success;
            }
            return BTStatus::Failure;
        }

        BTStatus ActRetreat(BTContext& ctx)
        {
            if (!ctx.bb->hasTarget || !ctx.self) return BTStatus::Failure;

            const TankLogicView selfView = ctx.self->GetLogicView();
            for (const auto& other : *ctx.allTanks)
            {
                if (!other || other->GetId() != ctx.bb->targetId) continue;
                const TankLogicView targetView = other->GetLogicView();
                const Pos selfX = selfView.position.x;
                const Pos selfY = selfView.position.y;
                const Pos targetX = targetView.position.x;
                const Pos targetY = targetView.position.y;
                int32_t dirXQ15 = kAngleSinCosScale;
                int32_t dirYQ15 = 0;
                NormalizeDeltaQ15(
                    static_cast<int64_t>(selfX) - static_cast<int64_t>(targetX),
                    static_cast<int64_t>(selfY) - static_cast<int64_t>(targetY),
                    dirXQ15,
                    dirYQ15);

                FixedVec2 retreatGoal = OffsetFromFixedPos(
                    selfX, selfY, dirXQ15, dirYQ15,
                    static_cast<Pos>(kAiRetreatDistancePosValue));
                AiPathFollower::MoveToward(
                    ctx,
                    ctx.navGrid,
                    retreatGoal,
                    AiMoveMode::RetreatFromTarget,
                    kAiSpeedScaleFullQ15,
                    ctx.bb->targetId);
                return BTStatus::Success;
            }
            return BTStatus::Failure;
        }

        BTStatus ActStrafe(BTContext& ctx)
        {
            if (!ctx.bb->hasTarget || !ctx.self || !ctx.memory) return BTStatus::Failure;

            ctx.memory->strafeSwitchFramesRemaining -= 1;
            if (ctx.memory->strafeSwitchFramesRemaining <= 0)
            {
                ctx.memory->strafeSwitchFramesRemaining = kAiStrafeSwitchFrames;
                if (ctx.gameRng)
                {
                    ctx.memory->strafeSign =
                        ctx.gameRng->UniformInt(
                            ctx.frameIndex,
                            RngPurpose::AiStrafeSign,
                            ctx.self->GetId(),
                            0,
                            2) == 0 ? -1 : 1;
                }
            }

            const TankLogicView selfView = ctx.self->GetLogicView();
            for (const auto& other : *ctx.allTanks)
            {
                if (!other || other->GetId() != ctx.bb->targetId) continue;
                const TankLogicView targetView = other->GetLogicView();
                const Pos selfX = selfView.position.x;
                const Pos selfY = selfView.position.y;
                const Pos targetX = targetView.position.x;
                const Pos targetY = targetView.position.y;
                int32_t toTargetXQ15 = kAngleSinCosScale;
                int32_t toTargetYQ15 = 0;
                NormalizeDeltaQ15(
                    static_cast<int64_t>(targetX) - static_cast<int64_t>(selfX),
                    static_cast<int64_t>(targetY) - static_cast<int64_t>(selfY),
                    toTargetXQ15,
                    toTargetYQ15);

                const int32_t sideXQ15 = (-toTargetYQ15) * ctx.memory->strafeSign;
                const int32_t sideYQ15 = toTargetXQ15 * ctx.memory->strafeSign;
                FixedVec2 strafeGoal = OffsetFromFixedPos(
                    selfX, selfY, sideXQ15, sideYQ15,
                    static_cast<Pos>(kAiStrafeOffsetPosValue));
                AiPathFollower::MoveToward(
                    ctx,
                    ctx.navGrid,
                    strafeGoal,
                    AiMoveMode::StrafeTarget,
                    kAiSpeedScaleThreeQuarterQ15,
                    ctx.bb->targetId);
                return BTStatus::Success;
            }
            return BTStatus::Failure;
        }

        BTStatus ActWander(BTContext& ctx)
        {
            if (!ctx.memory) return BTStatus::Failure;
            AiPathFollower::WanderOnGrid(ctx, ctx.navGrid, kAiSpeedScaleHalfQ15);
            return BTStatus::Success;
        }

        BTStatus CondHpLow(BTContext& ctx)
        {
            return ctx.bb->selfHp * 100 < ctx.bb->selfMaxHp * 35
                ? BTStatus::Success : BTStatus::Failure;
        }

        BTStatus CondTooFar(BTContext& ctx)
        {
            return ctx.bb->distToTargetSq > ctx.bb->engageRangeSq
                ? BTStatus::Success : BTStatus::Failure;
        }

        BTStatus CondTooClose(BTContext& ctx)
        {
            return ctx.bb->distToTargetSq < ctx.bb->retreatRangeSq
                ? BTStatus::Success : BTStatus::Failure;
        }
    }

    std::unique_ptr<BTNode> BuildDefaultTankBehaviorTree()
    {
        auto root = std::make_unique<BTSelector>();

        {
            auto emergency = std::make_unique<BTSequence>();
            emergency->AddChild(std::make_unique<BTCondition>(CondHpLow));
            emergency->AddChild(std::make_unique<BTAction>(ActUseAbility));
            root->AddChild(std::move(emergency));
        }

        {
            auto combat = std::make_unique<BTSequence>();
            combat->AddChild(std::make_unique<BTAction>(ActSelectTarget));

            auto positioning = std::make_unique<BTSelector>();
            {
                auto approach = std::make_unique<BTSequence>();
                approach->AddChild(std::make_unique<BTCondition>(CondTooFar));
                approach->AddChild(std::make_unique<BTAction>(ActApproach));
                positioning->AddChild(std::move(approach));
            }
            {
                auto retreat = std::make_unique<BTSequence>();
                retreat->AddChild(std::make_unique<BTCondition>(CondTooClose));
                retreat->AddChild(std::make_unique<BTAction>(ActRetreat));
                positioning->AddChild(std::move(retreat));
            }
            positioning->AddChild(std::make_unique<BTAction>(ActStrafe));
            combat->AddChild(std::move(positioning));
            combat->AddChild(std::make_unique<BTAction>(ActFireIfReady));
            root->AddChild(std::move(combat));
        }

        root->AddChild(std::make_unique<BTAction>(ActWander));
        return root;
    }
}

#include "AiSteering.h"
#include "../Common/Constants.h"
#include "../Common/AngleLUT.h"
#include "../Common/FixedMath.h"
#include "../TankLogicView.h"

namespace TankBattle
{
    void AiSteering::ApplyObstacleAvoidance(
        AiIntent& intent,
        const Tank& self,
        const std::vector<ObstacleWall>& obstacles)
    {
        if (obstacles.empty())
            return;

        if (intent.moveMode == AiMoveMode::FollowPath)
            return;

        if (intent.speedScaleQ15 < kAiSpeedScaleMinQ15)
            return;

        const TankLogicView selfView = self.GetLogicView();
        const Pos originX = selfView.position.x;
        const Pos originY = selfView.position.y;
        const Pos radiusPos = TankCollisionRadiusPos(selfView.sizePos);
        const Pos lookaheadPos = static_cast<Pos>(
            static_cast<int64_t>(radiusPos) + static_cast<Pos>(kAiObstacleLookaheadPosValue));
        const Pos clearancePos = static_cast<Pos>(kAiObstacleClearancePosValue);
        const Pos penaltyUnitPos = static_cast<Pos>(kAiObstacleProbePenaltyPosValue);

        const Pos forwardHit = RaycastObstaclesFromAngle(
            originX, originY, intent.moveHeading, lookaheadPos, obstacles);
        if (forwardHit >= lookaheadPos - clearancePos)
            return;

        const int probeSigned[] = {
            static_cast<int>(kAngleProbeOffset),
            -static_cast<int>(kAngleProbeOffset),
            static_cast<int>(kAngleProbeOffset90),
            -static_cast<int>(kAngleProbeOffset90),
            static_cast<int>(kAngleProbeOffset135),
            -static_cast<int>(kAngleProbeOffset135)
        };

        Angle bestHeading = intent.moveHeading;
        Pos bestScore = forwardHit;

        for (int offset : probeSigned)
        {
            const Angle candidateHeading = AddAngle(intent.moveHeading, offset);
            const Pos hit = RaycastObstaclesFromAngle(
                originX, originY, candidateHeading, lookaheadPos, obstacles);
            const int offsetMag = offset < 0 ? -offset : offset;
            const Pos penalty = static_cast<Pos>(
                (static_cast<int64_t>(penaltyUnitPos) * offsetMag) / kAngleProbeOffset);
            const Pos score = hit - penalty;
            if (score > bestScore)
            {
                bestScore = score;
                bestHeading = candidateHeading;
            }
        }

        if (bestHeading == intent.moveHeading)
            return;

        intent.moveHeading = bestHeading;
        intent.moveMode = AiMoveMode::AvoidObstacle;
        if (intent.speedScaleQ15 < kAiSpeedScaleLowQ15)
            intent.speedScaleQ15 = kAiSpeedScaleHalfQ15;
    }
}

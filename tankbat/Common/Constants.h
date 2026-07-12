#pragma once
#include <cstdint>

namespace TankBattle
{
    // 阵营定义（四角出生：苏 SW、美 SE、德 NW、意 NE）
    enum class Faction
    {
        Soviet = 0,
        USA = 1,
        Germany = 2,
        Italy = 3
    };

    constexpr int kFactionCount = 4;

    inline bool AreHostileFactions(Faction a, Faction b)
    {
        return a != b;
    }
    
    // 坦克类型
    enum class TankType
    {
        T34 = 0,    // 苏联
        KV1 = 1,
        KV2 = 2,
        M3 = 3,     // 美国
        M4 = 4,
        Panther = 5, // 德国
        Tiger = 6,
        L6_40 = 7,   // 意大利
        P40 = 8
    };
    
    // 特殊能力类型
    enum class AbilityType
    {
        None = 0,
        Heal = 1,      // 治疗
        Shield = 2,    // 护盾
        SpeedBoost = 3,// 加速
        RapidFire = 4, // 快速射击
        AOE = 5,       // 范围攻击
        Penetrate = 6, // 穿透
        Snipe = 7      // 狙击
    };
    
    // 游戏状态
    enum class GameState
    {
        Waiting = 0,
        Playing = 1,
        Ended = 2
    };

    // 非玩家坦克 AI 位移意图（写入 TankState，供 Unity 等客户端调试输出）
    enum class AiMoveMode : uint32_t
    {
        None = 0,
        WanderNoTarget = 1,
        ApproachTarget = 2,
        RetreatFromTarget = 3,
        StrafeTarget = 4,
        AvoidObstacle = 5,
        FollowPath = 6
    };

    // 整数 RNG / 地图：角度 0..65535；世界坐标 1 逻辑单位 = 64 Pos 子单位
    constexpr int kRngAngleUnits = 65536;
    constexpr int kRngWorldSubunitsPerUnit = 64;

    constexpr int kNavGridCellSizePosValue = 25 * kRngWorldSubunitsPerUnit;
    constexpr int kDefaultWorldSizePosValue = 1000 * kRngWorldSubunitsPerUnit;
    constexpr int kNavGridWallInflateRadiusPosValue = 18 * kRngWorldSubunitsPerUnit;
    constexpr int kNavGridAgentRadiusPosValue = 28 * kRngWorldSubunitsPerUnit;
    // A* 整数代价：直走 1000，对角 1414（≈1000*√2）
    constexpr int32_t kNavGridStraightCost = 1000;
    constexpr int32_t kNavGridDiagonalCost = 1414;
    constexpr int32_t kNavGridHeuristicDiagFactor = 414; // (sqrt2-1)*1000
    constexpr int32_t kNavGridCostInf = 0x3FFFFFFF;
    constexpr int kAiPathWaypointReachPosValue = 22 * kRngWorldSubunitsPerUnit;
    constexpr int kAiPathGoalMoveThresholdPosValue = 40 * kRngWorldSubunitsPerUnit;
    constexpr int kAiRetreatDistancePosValue = 150 * kRngWorldSubunitsPerUnit;
    constexpr int kAiStrafeOffsetPosValue = 90 * kRngWorldSubunitsPerUnit;
    constexpr int kAiEngageRangeDefaultPosValue = 120 * kRngWorldSubunitsPerUnit;
    constexpr int kAiEngageRangeTigerPosValue = 160 * kRngWorldSubunitsPerUnit;
    constexpr int kAiEngageRangeKv2PosValue = 100 * kRngWorldSubunitsPerUnit;
    constexpr int kAiRetreatRangeDefaultPosValue = 80 * kRngWorldSubunitsPerUnit;
    constexpr int kAiRetreatRangeTigerPosValue = 100 * kRngWorldSubunitsPerUnit;
    constexpr int kAiRetreatRangeKv2PosValue = 60 * kRngWorldSubunitsPerUnit;
    constexpr int kAiObstacleLookaheadPosValue = 90 * kRngWorldSubunitsPerUnit;
    constexpr int kAiObstacleClearancePosValue = 12 * kRngWorldSubunitsPerUnit;
    constexpr int kAiObstacleProbePenaltyPosValue = 8 * kRngWorldSubunitsPerUnit;
    constexpr int kMinRespawnEnemyDistancePosValue = 150 * kRngWorldSubunitsPerUnit;
    constexpr int kFactionSpawnMarginPosValue = 80 * kRngWorldSubunitsPerUnit;
    constexpr int kFactionSpawnSpreadPosValue = 140 * kRngWorldSubunitsPerUnit;

    // 逻辑帧计数（30Hz，用于确定性计时，替代 float 秒倒计时）
    constexpr int kLogicFramesPerSecond = 30;
    constexpr int kPlayerRespawnFrames = 150;           // 5.0s
    constexpr int kPlayerSpawnProtectionFrames = 75;    // 2.5s
    constexpr int kShieldDurationFrames = 150;          // 5.0s
    constexpr int kSpeedBoostDurationFrames = 150;      // 5.0s
    constexpr int kRapidFireDurationFrames = 150;       // 5.0s
    constexpr int kAbilityCooldownFrames = 300;         // 10.0s
    constexpr int kBulletLifeFrames = 90;               // 3.0s
    constexpr int kSnipeBulletLifeFrames = 150;         // 5.0s
    constexpr int kAiStrafeSwitchFrames = 45;           // 1.5s
    constexpr int kAiPathRecalcFrames = 15;             // 0.5s
    constexpr int kAiWanderGoalFrames = 90;             // 3.0s
    constexpr int kRapidFireReloadNumerator = 55;
    constexpr int kRapidFireReloadDenominator = 100;
    constexpr int kShieldDamageNumerator = 50;
    constexpr int kShieldDamageDenominator = 100;
    constexpr int kPenetrateDamageNumerator = 135;
    constexpr int kPenetrateDamageDenominator = 100;
    constexpr int kSnipeDamageNumerator = 175;
    constexpr int kSnipeDamageDenominator = 100;
    constexpr int kHealAmount = 150;

    constexpr int kDefaultTankSizePosValue = 20 * kRngWorldSubunitsPerUnit;
    constexpr int kBulletMuzzleOffsetPosValue = 10 * kRngWorldSubunitsPerUnit;
    constexpr int kSpeedBoostNumerator = 3;
    constexpr int kSpeedBoostDenominator = 2;
    constexpr int kRapidFireBulletSpeedNumerator = 6;
    constexpr int kRapidFireBulletSpeedDenominator = 5;
    constexpr int kAiSpeedScaleMinQ15 = 1638;
    constexpr int kAiSpeedScaleLowQ15 = 3277;
    constexpr int kAiSpeedScaleHalfQ15 = 16383;
    constexpr int kAiSpeedScaleThreeQuarterQ15 = 24575;
    constexpr int kAiSpeedScaleFullQ15 = 32767;

    constexpr float kTwoPi = 6.283185307f;

    inline float AngleUnitsToRadians(int angleUnits)
    {
        return static_cast<float>(angleUnits) * (kTwoPi / static_cast<float>(kRngAngleUnits));
    }

    inline float LogicFramesToSeconds(int frames)
    {
        return static_cast<float>(frames) / static_cast<float>(kLogicFramesPerSecond);
    }

    inline uint64_t LogicFrameToMilliseconds(uint32_t frame)
    {
        return static_cast<uint64_t>(frame) * 1000 / static_cast<uint64_t>(kLogicFramesPerSecond);
    }

    // 坦克互阻：碰撞半径 = sizePos * 92 / 100
    constexpr int kTankCollisionRadiusNumerator = 92;
    constexpr int kTankCollisionRadiusDenominator = 100;
    constexpr int kTankOverlapResolveIterations = 2;

    // 权威逻辑帧率（帧同步 / GameCore::Update 固定步长）
    constexpr float kFixedLogicDeltaTime = 1.0f / 30.0f;
    
    // 网络消息类型
    enum class MessageType
    {
        Connect = 0,
        PlayerInput = 1,
        GameStateUpdate = 2,
        PlayerJoined = 3,
        PlayerLeft = 4,
        GameStart = 5,
        GameEnd = 6,
        AbilityUsed = 7
    };
}
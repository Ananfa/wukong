#pragma once
#include <cstdint>
#include <string>
#include <vector>

#include "Constants.h"
#include "FixedMath.h"

namespace TankBattle
{
    // 向量结构
    struct Vector2
    {
        float x = 0.0f;
        float y = 0.0f;

        Vector2() = default;
        Vector2(float x, float y) : x(x), y(y) {}
    };

    inline Vector2 FixedVec2ToVector2(const FixedVec2& fixed)
    {
        return {PosToWorld(fixed.x), PosToWorld(fixed.y)};
    }

    inline Vector2 VelPerFrameToWorldVelocity(const FixedVel2& velPerFrame)
    {
        return {
            VelPerFrameToWorldSpeed(velPerFrame.x),
            VelPerFrameToWorldSpeed(velPerFrame.y)
        };
    }
    
    // 玩家输入：方向分量为 Q15 幅度 int16（±32767 = 满幅），与 kAngleSinCosScale 一致
    struct PlayerInput
    {
        uint32_t playerId = 0;
        uint32_t frame = 0;
        int16_t moveX = 0;
        int16_t moveY = 0;
        int16_t aimX = 0;
        int16_t aimY = 0;
        bool fire = false;
        bool useAbility = false;
        uint64_t timestamp = 0;
    };
    
    // 坦克状态
    struct TankState
    {
        uint32_t id = 0;
        uint32_t playerId = 0;
        Faction faction = Faction::Soviet;
        TankType type = TankType::T34;
        Vector2 position{0.0f, 0.0f};
        Vector2 velocity{0.0f, 0.0f};
        float rotation = 0.0f;       // 车身旋转
        float turretRotation = 0.0f; // 炮塔旋转
        float hp = 0.0f;
        float maxHp = 0.0f;
        float shield = 0.0f;         // 护盾剩余时间
        float speedBoost = 0.0f;     // 加速剩余时间
        float rapidFire = 0.0f;      // 速射剩余时间
        float abilityCooldown = 0.0f; // 技能冷却
        bool isAlive = true;
        uint32_t lockedTargetId = 0; // 锁定目标ID
        uint32_t aiMoveMode = 0;     // AiMoveMode，仅 AI 坦克有效
        float moveSpeed = 0.0f;      // 配置最大移动速度（Unity 快照边界）
        float respawnTimeRemaining = 0.0f; // 玩家死亡后剩余复活秒数，0 表示不在等待
        float reloadTimeRemaining = 0.0f;  // 装填/准备下一炮剩余秒数，0 表示可开火
        float reloadDuration = 0.0f;       // 本次装填总时长（用于 UI 进度）
    };
    
    // 子弹状态（逻辑层定点坐标；导出快照时再换算为 float 世界坐标）
    struct BulletState
    {
        uint32_t id = 0;
        uint32_t ownerId = 0;
        FixedVec2 position;
        FixedVel2 velocity;
        int32_t damage = 0;
        int32_t lifeFramesRemaining = 0;
        bool penetrating = false;
        std::vector<uint32_t> damagedTankIds;
    };
    
    // 阻挡墙（Native 坐标，中心 + 宽高 + 弧度旋转）
    struct ObstacleWallState
    {
        float centerX = 0.0f;
        float centerY = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        float rotation = 0.0f;
    };

    // 阵营出生/复活矩形区域（Pos 子单位，含边界）
    struct FactionSpawnZone
    {
        Pos minX = 0;
        Pos maxX = 0;
        Pos minY = 0;
        Pos maxY = 0;
    };
    
    // 游戏快照
    struct GameSnapshot
    {
        uint32_t frame = 0;
        uint64_t timestamp = 0;
        std::vector<TankState> tanks;
        std::vector<BulletState> bullets;
        std::vector<ObstacleWallState> obstacles;
        GameState state = GameState::Waiting;
    };
    
    // 玩家信息
    struct PlayerInfo
    {
        uint32_t id = 0;
        std::string name;
        Faction faction = Faction::Soviet;
        uint32_t kills = 0;
        uint32_t score = 0;
        bool isConnected = false;
    };
    
    // 阵营状态
    struct FactionStatus
    {
        Faction faction = Faction::Soviet;
        uint32_t aliveCount = 0;
        uint32_t totalCount = 0;
        uint32_t kills = 0;   // 阵营累计击毁敌对阵营单位数
        uint32_t deaths = 0;  // 阵营累计被击毁次数（含复活后再次死亡）
    };
}
#pragma once
#include "../Common/FixedMath.h"
#include "../Common/AngleLUT.h"
#include "../Common/Types.h"
#include "../Common/Constants.h"
#include "Ai/AiIntent.h"
#include "TankLogicView.h"
#include <cstdint>
#include <memory>

namespace TankBattle
{
    class GameCore;
    
    class Tank
    {
    public:
        Tank(uint32_t id, uint32_t playerId, Faction faction, TankType type, 
             const FixedVec2& position, bool isPlayer = false, Angle initialRotation = 0);
        
        // 更新坦克状态
        void Update(const std::vector<std::shared_ptr<Tank>>& allTanks,
                    int16_t moveX = 0, int16_t moveY = 0);
        
        // 应用伤害
        void TakeDamage(int32_t damage, uint32_t attackerId);
        
        // 开火
        std::shared_ptr<BulletState> Fire();
        
        // 使用特殊能力
        void UseAbility();
        
        // 获取状态
        TankState GetState() const;

        // AI / 逻辑层只读定点视图（不经 float 快照）
        TankLogicView GetLogicView() const;
        
        // 设置AI
        void SetAI(bool isAI) { m_isAI = isAI; }
        
        // 玩家炮塔瞄准（Q15 方向分量，与 moveX/moveY 同一坐标系）
        void SetTurretAim(int16_t aimX, int16_t aimY);
        
        // 获取ID
        uint32_t GetId() const { return m_id; }
        uint32_t GetPlayerId() const { return m_playerId; }
        Faction GetFaction() const { return m_faction; }
        bool IsAlive() const { return m_hp > 0; }
        int32_t GetHP() const { return m_hp; }
        int32_t GetMaxHP() const { return m_maxHp; }
        bool CanFire() const { return m_reloadFramesRemaining <= 0; }
        bool IsAbilityReady() const { return m_abilityCooldownFramesRemaining <= 0 && m_isAlive; }

        // 由 GameCore BT 调用：应用移动意图（不含开火/技能）
        void ApplyAiIntent(const AiIntent& intent);

        // 玩家在复活点重生（GameCore 调用）
        void RespawnAt(const FixedVec2& position, Angle initialRotation = 0);
        
    private:
        // 坦克配置
        struct TankConfig
        {
            int32_t maxHp = 100;
            Vel moveVelPerFrame = 0;
            Pos sizePos = 0;
            int reloadFrames = 30;
            Vel recoilKickVel = 0;
            Vel bulletVelPerFrame = 0;
            int32_t damage = 30;
            AbilityType ability = AbilityType::None;
            uint32_t color = 0xFF0000FF; // RGBA
        };
        
        // 获取坦克配置
        TankConfig GetTankConfig() const;

        Vel GetEffectiveMoveVelPerFrame() const;

        // 玩家更新
        void PlayerUpdate(int16_t moveX, int16_t moveY);

        void TickFrameTimers();
        
    private:
        uint32_t m_id = 0;
        uint32_t m_playerId = 0;
        Faction m_faction = Faction::Soviet;
        TankType m_type = TankType::T34;
        FixedVec2 m_position;
        FixedVel2 m_velocity;
        Angle m_rotation = 0;
        Angle m_turretRotation = 0;
        int32_t m_hp = 0;
        int32_t m_maxHp = 0;
        int m_shieldFramesRemaining = 0;
        int m_speedBoostFramesRemaining = 0;
        int m_rapidFireFramesRemaining = 0;
        int m_abilityCooldownFramesRemaining = 0;
        int m_reloadFramesRemaining = 0;
        int m_lastReloadDurationFrames = 0;
        FixedVel2 m_recoilVelocity;
        TankConfig m_config;
        uint32_t m_lockedTargetId = 0;  // 锁定目标ID
        uint32_t m_aiMoveMode = 0;      // AiMoveMode，AI 位移分支
        int m_respawnFramesRemaining = 0;
        int m_spawnProtectionFramesRemaining = 0;
        bool m_chargedShot = false;     // 技能充能：下一发炮弹带穿透/狙击加成
        bool m_isPlayer = false;
        bool m_isAI = true;
        bool m_isAlive = true;
        
        static uint32_t s_nextBulletId;

    private:
        friend class GameCore;
    };
}

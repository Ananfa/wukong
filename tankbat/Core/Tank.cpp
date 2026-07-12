#include "Tank.h"
#include "../Common/AngleLUT.h"
#include <algorithm>

namespace TankBattle
{
    uint32_t Tank::s_nextBulletId = 1;
    
Tank::Tank(uint32_t id, uint32_t playerId, Faction faction, TankType type, 
           const FixedVec2& position, bool isPlayer, Angle initialRotation)
    : m_id(id)
    , m_playerId(playerId)
    , m_faction(faction)
    , m_type(type)
    , m_position(position)
    , m_isPlayer(isPlayer)
    , m_isAI(!isPlayer)
{
    m_config = GetTankConfig();
    m_maxHp = m_config.maxHp;
    m_hp = m_maxHp;
    m_rotation = initialRotation;
    m_turretRotation = m_rotation;
}

void Tank::Update(const std::vector<std::shared_ptr<Tank>>& allTanks,
                 int16_t moveX, int16_t moveY)
{
    if (!m_isAlive || m_hp <= 0) return;
    
    TickFrameTimers();
    
    if (m_isPlayer)
        PlayerUpdate(moveX, moveY);

    if (RecoilVelIsActive(m_recoilVelocity))
    {
        m_position.x += m_recoilVelocity.x;
        m_position.y += m_recoilVelocity.y;
        ApplyRecoilDecay(m_recoilVelocity);
    }
    else
    {
        m_recoilVelocity = {0, 0};
    }
    
    if (m_lockedTargetId > 0)
    {
        auto it = std::find_if(allTanks.begin(), allTanks.end(),
            [this](const std::shared_ptr<Tank>& tank) {
                return tank->GetId() == m_lockedTargetId && tank->IsAlive();
            });
        
        if (it != allTanks.end())
        {
            if (AreHostileFactions(m_faction, (*it)->GetFaction()))
            {
                const FixedVec2& targetPos = (*it)->m_position;
                const int32_t dx = targetPos.x - m_position.x;
                const int32_t dy = targetPos.y - m_position.y;
                m_turretRotation = Atan2Pos(dy, dx);
            }
            else
            {
                m_lockedTargetId = 0;
            }
        }
        else
        {
            m_lockedTargetId = 0;
        }
    }
}

void Tank::TickFrameTimers()
{
    if (m_shieldFramesRemaining > 0) --m_shieldFramesRemaining;
    if (m_speedBoostFramesRemaining > 0) --m_speedBoostFramesRemaining;
    if (m_rapidFireFramesRemaining > 0) --m_rapidFireFramesRemaining;
    if (m_abilityCooldownFramesRemaining > 0) --m_abilityCooldownFramesRemaining;
    if (m_reloadFramesRemaining > 0) --m_reloadFramesRemaining;
    if (m_spawnProtectionFramesRemaining > 0) --m_spawnProtectionFramesRemaining;
}

Vel Tank::GetEffectiveMoveVelPerFrame() const
{
    Vel vel = m_config.moveVelPerFrame;
    if (m_speedBoostFramesRemaining > 0)
        vel = static_cast<Vel>(
            (static_cast<int64_t>(vel) * kSpeedBoostNumerator) / kSpeedBoostDenominator);
    return vel;
}

void Tank::ApplyAiIntent(const AiIntent& intent)
{
    m_lockedTargetId = intent.targetId;
    m_aiMoveMode = static_cast<uint32_t>(intent.moveMode);
    m_rotation = intent.moveHeading;
    if (intent.speedScaleQ15 > 0)
    {
        m_velocity = VelFromAngleVelScaledQ15(
            intent.moveHeading,
            GetEffectiveMoveVelPerFrame(),
            intent.speedScaleQ15);
    }
    else
    {
        m_velocity = {0, 0};
    }
    m_position.x += m_velocity.x;
    m_position.y += m_velocity.y;
}

void Tank::RespawnAt(const FixedVec2& position, Angle initialRotation)
{
    m_position = position;
    m_velocity = {0, 0};
    m_hp = m_maxHp;
    m_isAlive = true;
    m_respawnFramesRemaining = 0;
    m_shieldFramesRemaining = 0;
    m_speedBoostFramesRemaining = 0;
    m_rapidFireFramesRemaining = 0;
    m_reloadFramesRemaining = 0;
    m_lastReloadDurationFrames = 0;
    m_recoilVelocity = {0, 0};
    m_lockedTargetId = 0;
    m_aiMoveMode = static_cast<uint32_t>(AiMoveMode::None);
    m_chargedShot = false;
    m_rotation = initialRotation;
    m_turretRotation = m_rotation;
    if (m_isPlayer && m_playerId != 0)
        m_spawnProtectionFramesRemaining = kPlayerSpawnProtectionFrames;
}

void Tank::SetTurretAim(int16_t aimX, int16_t aimY)
{
    const int64_t lenSq =
        static_cast<int64_t>(aimX) * aimX + static_cast<int64_t>(aimY) * aimY;
    if (lenSq > kAngleSinCosScale)
        m_turretRotation = Atan2Pos(aimY, aimX);
}

void Tank::TakeDamage(int32_t damage, uint32_t attackerId)
{
    if (!m_isAlive || m_spawnProtectionFramesRemaining > 0) return;
    
    int32_t actualDamage = damage;
    if (m_shieldFramesRemaining > 0)
        actualDamage = actualDamage * kShieldDamageNumerator / kShieldDamageDenominator;
    
    m_hp -= actualDamage;
    if (m_hp <= 0)
    {
        m_hp = 0;
        m_isAlive = false;
        if (m_isPlayer && m_playerId != 0)
            m_respawnFramesRemaining = kPlayerRespawnFrames;
    }
}

std::shared_ptr<BulletState> Tank::Fire()
{
    if (m_reloadFramesRemaining > 0) return nullptr;
    
    int reloadFrames = m_config.reloadFrames;
    if (m_rapidFireFramesRemaining > 0)
        reloadFrames = reloadFrames * kRapidFireReloadNumerator / kRapidFireReloadDenominator;
    m_reloadFramesRemaining = reloadFrames;
    m_lastReloadDurationFrames = reloadFrames;

    FixedVel2 kickVel = VelFromAngleVelPerFrame(m_turretRotation, m_config.recoilKickVel);
    m_recoilVelocity.x -= kickVel.x;
    m_recoilVelocity.y -= kickVel.y;
    
    auto bullet = std::make_shared<BulletState>();
    bullet->id = s_nextBulletId++;
    bullet->ownerId = m_id;
    
    OffsetFixedPosition(
        m_position,
        m_turretRotation,
        m_config.sizePos + static_cast<Pos>(kBulletMuzzleOffsetPosValue),
        bullet->position);
    
    Vel bulletVel = m_config.bulletVelPerFrame;
    if (m_rapidFireFramesRemaining > 0)
    {
        bulletVel = static_cast<Vel>(
            (static_cast<int64_t>(bulletVel) * kRapidFireBulletSpeedNumerator)
            / kRapidFireBulletSpeedDenominator);
    }
    
    bullet->velocity = VelFromAngleVelPerFrame(m_turretRotation, bulletVel);
    
    bullet->damage = m_config.damage;
    bullet->lifeFramesRemaining = kBulletLifeFrames;
    
    if (m_chargedShot)
    {
        switch (m_config.ability)
        {
        case AbilityType::Penetrate:
            bullet->penetrating = true;
            bullet->damage = bullet->damage * kPenetrateDamageNumerator / kPenetrateDamageDenominator;
            break;
        case AbilityType::Snipe:
            bullet->lifeFramesRemaining = kSnipeBulletLifeFrames;
            bullet->damage = bullet->damage * kSnipeDamageNumerator / kSnipeDamageDenominator;
            bullet->velocity = ScaleVelPerFrame(bullet->velocity, 3, 2);
            break;
        default:
            break;
        }
        m_chargedShot = false;
    }
    
    return bullet;
}

void Tank::UseAbility()
{
    if (m_abilityCooldownFramesRemaining > 0 || !m_isAlive) return;
    
    m_abilityCooldownFramesRemaining = kAbilityCooldownFrames;
    
    switch (m_config.ability)
    {
    case AbilityType::Heal:
    {
        int32_t newHp = m_hp + kHealAmount;
        m_hp = newHp < m_maxHp ? newHp : m_maxHp;
        break;
    }
    case AbilityType::Shield:
        m_shieldFramesRemaining = kShieldDurationFrames;
        break;
        
    case AbilityType::SpeedBoost:
        m_speedBoostFramesRemaining = kSpeedBoostDurationFrames;
        break;
        
    case AbilityType::RapidFire:
        m_rapidFireFramesRemaining = kRapidFireDurationFrames;
        break;
        
    case AbilityType::AOE:
        break;
        
    case AbilityType::Penetrate:
    case AbilityType::Snipe:
        m_chargedShot = true;
        break;
    default:
        break;
    }
}

void Tank::PlayerUpdate(int16_t moveX, int16_t moveY)
{
    const int32_t ix = moveX;
    const int32_t iy = moveY;
    const int64_t lenSq = static_cast<int64_t>(ix) * ix + static_cast<int64_t>(iy) * iy;
    const int64_t thresholdSq =
        static_cast<int64_t>(kAngleSinCosScale) * kAngleSinCosScale / 100;
    if (lenSq > thresholdSq)
    {
        const Vel speedVel = GetEffectiveMoveVelPerFrame();
        m_velocity = {
            static_cast<Vel>((static_cast<int64_t>(speedVel) * ix) / kAngleSinCosScale),
            static_cast<Vel>((static_cast<int64_t>(speedVel) * iy) / kAngleSinCosScale)
        };
        m_rotation = Atan2Pos(iy, ix);
        
        m_position.x += m_velocity.x;
        m_position.y += m_velocity.y;
    }
    else
    {
        m_velocity = {0, 0};
    }
}

TankLogicView Tank::GetLogicView() const
{
    TankLogicView view;
    view.id = m_id;
    view.playerId = m_playerId;
    view.faction = m_faction;
    view.type = m_type;
    view.position = m_position;
    view.rotation = m_rotation;
    view.hp = m_hp;
    view.maxHp = m_maxHp;
    view.isAlive = m_isAlive;
    view.sizePos = m_config.sizePos;
    return view;
}

TankState Tank::GetState() const
{
    TankState state;
    state.id = m_id;
    state.playerId = m_playerId;
    state.faction = m_faction;
    state.type = m_type;
    state.position = FixedVec2ToVector2(m_position);
    state.velocity = VelPerFrameToWorldVelocity(m_velocity);
    state.rotation = AngleToRadians(m_rotation);
    state.turretRotation = AngleToRadians(m_turretRotation);
    state.hp = static_cast<float>(m_hp);
    state.maxHp = static_cast<float>(m_maxHp);
    state.shield = LogicFramesToSeconds(m_shieldFramesRemaining);
    state.speedBoost = LogicFramesToSeconds(m_speedBoostFramesRemaining);
    state.rapidFire = LogicFramesToSeconds(m_rapidFireFramesRemaining);
    state.abilityCooldown = LogicFramesToSeconds(m_abilityCooldownFramesRemaining);
    state.isAlive = m_isAlive;
    state.lockedTargetId = m_lockedTargetId;
    state.aiMoveMode = m_isAI ? m_aiMoveMode : 0U;
    state.moveSpeed = VelPerFrameToWorldSpeed(m_config.moveVelPerFrame);
    state.respawnTimeRemaining = (!m_isAlive && m_isPlayer && m_playerId != 0)
        ? LogicFramesToSeconds(m_respawnFramesRemaining) : 0.0f;
    state.reloadTimeRemaining = (m_reloadFramesRemaining > 0)
        ? LogicFramesToSeconds(m_reloadFramesRemaining) : 0.0f;
    state.reloadDuration = LogicFramesToSeconds(m_lastReloadDurationFrames);
    return state;
}

Tank::TankConfig Tank::GetTankConfig() const
{
    TankConfig config;
    config.sizePos = static_cast<Pos>(kDefaultTankSizePosValue);
    
    switch (m_faction)
    {
    case Faction::Soviet:
        switch (m_type)
        {
        case TankType::T34:
            config.maxHp = 220;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(32);
            config.reloadFrames = 45;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(55);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(90);
            config.damage = 35;
            config.ability = AbilityType::Heal;
            config.color = 0xDC143CFF;
            break;
            
        case TankType::KV1:
            config.maxHp = 300;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(20);
            config.reloadFrames = 54;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(70);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(80);
            config.damage = 45;
            config.ability = AbilityType::Shield;
            config.color = 0xB22222FF;
            break;
            
        case TankType::KV2:
            config.maxHp = 500;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(15);
            config.reloadFrames = 66;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(95);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(70);
            config.damage = 60;
            config.ability = AbilityType::AOE;
            config.color = 0x8B0000FF;
            break;
        default:
            break;
        }
        break;
        
    case Faction::USA:
        switch (m_type)
        {
        case TankType::M3:
            config.maxHp = 180;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(38);
            config.reloadFrames = 36;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(45);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(100);
            config.damage = 25;
            config.ability = AbilityType::SpeedBoost;
            config.color = 0x1E90FFFF;
            break;
            
        case TankType::M4:
            config.maxHp = 250;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(30);
            config.reloadFrames = 42;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(58);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(90);
            config.damage = 30;
            config.ability = AbilityType::RapidFire;
            config.color = 0x4169E1FF;
            break;
        default:
            break;
        }
        break;
        
    case Faction::Germany:
        switch (m_type)
        {
        case TankType::Panther:
            config.maxHp = 200;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(35);
            config.reloadFrames = 48;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(62);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(110);
            config.damage = 38;
            config.ability = AbilityType::Penetrate;
            config.color = 0x696969FF;
            break;
            
        case TankType::Tiger:
            config.maxHp = 400;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(20);
            config.reloadFrames = 60;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(88);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(80);
            config.damage = 50;
            config.ability = AbilityType::Snipe;
            config.color = 0x2F4F4FFF;
            break;
        default:
            break;
        }
        break;
        
    case Faction::Italy:
        switch (m_type)
        {
        case TankType::L6_40:
            config.maxHp = 170;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(36);
            config.reloadFrames = 39;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(42);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(95);
            config.damage = 28;
            config.ability = AbilityType::SpeedBoost;
            config.color = 0x009246FF;
            break;
            
        case TankType::P40:
            config.maxHp = 230;
            config.moveVelPerFrame = WorldSpeedToVelPerFrameInt(31);
            config.reloadFrames = 45;
            config.recoilKickVel = WorldSpeedToVelPerFrameInt(56);
            config.bulletVelPerFrame = WorldSpeedToVelPerFrameInt(92);
            config.damage = 32;
            config.ability = AbilityType::RapidFire;
            config.color = 0x006400FF;
            break;
        default:
            break;
        }
        break;
    }
    
    return config;
}}

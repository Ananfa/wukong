#include "GameCore.h"
#include "Ai/AiController.h"
#include "MapObstacleLoader.h"
#include "ObstacleWall.h"
#include "../Common/FixedMath.h"
#include "../Common/AngleLUT.h"
#include <algorithm>
#include <limits>
#include <sstream>
#include <string>

namespace TankBattle
{
    GameCore::GameCore()
    {
        InitAngleLut();
        m_rng.SetSeed(1);
        LoadDefaultMapObstacles(m_obstacles, m_worldWidthPos, m_worldHeightPos);
        LoadDefaultSpawnZones(m_spawnZones);
        m_hasSpawnZones = true;
        RebuildNavigationGrid();
    }
    
    GameCore::~GameCore()
    {
    }
    
    bool GameCore::Initialize(uint32_t maxPlayers)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        Reset();
        m_state = GameState::Waiting;
        
        return true;
    }
    
    void GameCore::Update()
    {
        AdvanceSimulation();
    }

    uint32_t GameCore::GetFrame() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_frame;
    }

    bool GameCore::SetFrameInputs(uint32_t frame, const PlayerInput* inputs, size_t count)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_state != GameState::Playing)
            return false;
        if (frame != m_frame + 1)
            return false;

        m_frameInputs.clear();
        if (inputs != nullptr)
        {
            for (size_t i = 0; i < count; ++i)
            {
                const PlayerInput& src = inputs[i];
                if (m_players.find(src.playerId) == m_players.end())
                    continue;
                if (!m_players[src.playerId].isConnected)
                    continue;

                PlayerInput stored = src;
                stored.frame = frame;
                m_frameInputs[src.playerId] = stored;
            }
        }
        return true;
    }

    void GameCore::ApplyPlayerFrameInputs()
    {
        for (const auto& entry : m_frameInputs)
        {
            const uint32_t playerId = entry.first;
            const PlayerInput& input = entry.second;

            auto tankIt = std::find_if(m_tanks.begin(), m_tanks.end(),
                [playerId](const std::pair<uint32_t, std::shared_ptr<Tank>>& pair) {
                    return pair.second->GetPlayerId() == playerId;
                });

            if (tankIt == m_tanks.end() || !tankIt->second->IsAlive())
                continue;

            tankIt->second->Update({}, input.moveX, input.moveY);

            const int64_t aimLenSq =
                static_cast<int64_t>(input.aimX) * input.aimX
                + static_cast<int64_t>(input.aimY) * input.aimY;
            if (aimLenSq > kAngleSinCosScale)
                tankIt->second->SetTurretAim(input.aimX, input.aimY);

            if (input.fire)
            {
                auto bullet = tankIt->second->Fire();
                if (bullet)
                    m_bullets.push_back(bullet);
            }

            if (input.useAbility)
                tankIt->second->UseAbility();
        }
    }

    void GameCore::AdvanceSimulation()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_state != GameState::Playing)
            return;

        ++m_frame;
        ApplyPlayerFrameInputs();
        m_frameInputs.clear();

        UpdateTanks();
        ResolveTankOverlaps();
        ResolveObstacleCollisions();
        UpdateBullets();
        UpdatePlayerRespawns();

        CheckCollisions();
        CleanupDeadUnits();

        GenerateAITanks();

        CheckGameEnd();
    }
    
    uint32_t GameCore::AddPlayer(const std::string& name, Faction faction)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        uint32_t playerId = m_nextPlayerId++;
        PlayerInfo player;
        player.id = playerId;
        player.name = name;
        player.faction = faction;
        player.isConnected = true;
        
        m_players[playerId] = player;
        
            // 如果游戏正在进行中，为玩家创建坦克
            if (m_state == GameState::Playing)
        {
            uint32_t tankId = m_nextTankId++;
            auto tank = CreateTankInstance(tankId, playerId, faction, true);
            m_tanks[tankId] = tank;
        }
        
        return playerId;
    }
    
    void GameCore::RemovePlayer(uint32_t playerId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        auto it = m_players.find(playerId);
        if (it != m_players.end())
        {
            it->second.isConnected = false;
            
            // 移除玩家的坦克
            for (auto tankIt = m_tanks.begin(); tankIt != m_tanks.end();)
            {
                if (tankIt->second->GetPlayerId() == playerId)
                {
                    m_aiController.RemoveMemory(tankIt->first);
                    tankIt = m_tanks.erase(tankIt);
                }
                else
                {
                    ++tankIt;
                }
            }
            
            // 移除待处理的输入
            m_frameInputs.erase(playerId);
        }
    }

    void GameCore::SetSlotsPerFaction(uint32_t slots)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (slots < 1)
            slots = 1;
        if (slots > 16)
            slots = 16;
        m_slotsPerFaction = slots;
    }

    uint32_t GameCore::GetSlotsPerFaction() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_slotsPerFaction;
    }

    uint32_t GameCore::CountFreeAISlots(Faction faction) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        uint32_t aiCount = 0;
        uint32_t total = 0;
        for (const auto& entry : m_tanks)
        {
            const auto& tank = entry.second;
            if (tank->GetFaction() != faction)
                continue;
            ++total;
            if (tank->GetPlayerId() == 0)
                ++aiCount;
        }

        if (total < m_slotsPerFaction)
            return aiCount + (m_slotsPerFaction - total);
        return aiCount;
    }

    uint32_t GameCore::PossessAITank(const std::string& name, Faction faction, uint32_t* outTankId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (outTankId)
            *outTankId = 0;

        if (static_cast<int>(faction) < 0 || static_cast<int>(faction) >= kFactionCount)
            return 0;

        if (m_state == GameState::Playing)
            GenerateAITanks(); // 已持锁；补齐未刷满的槽位

        // 确定性：选同阵营 playerId==0 且 tankId 最小的一台
        std::shared_ptr<Tank> target;
        for (const auto& entry : m_tanks)
        {
            const auto& tank = entry.second;
            if (tank->GetFaction() != faction || tank->GetPlayerId() != 0)
                continue;
            if (!target || tank->GetId() < target->GetId())
                target = tank;
        }
        if (!target)
            return 0;

        uint32_t playerId = m_nextPlayerId++;
        PlayerInfo player;
        player.id = playerId;
        player.name = name;
        player.faction = faction;
        player.isConnected = true;
        m_players[playerId] = player;

        target->SetControlOwner(playerId, true);
        m_aiController.RemoveMemory(target->GetId());

        if (outTankId)
            *outTankId = target->GetId();
        return playerId;
    }

    bool GameCore::ReleaseToAI(uint32_t playerId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        bool released = false;
        for (auto& entry : m_tanks)
        {
            auto& tank = entry.second;
            if (tank->GetPlayerId() != playerId)
                continue;
            tank->SetControlOwner(0, false);
            released = true;
        }

        m_frameInputs.erase(playerId);
        auto pit = m_players.find(playerId);
        if (pit != m_players.end())
            m_players.erase(pit);
        (void)released;
        return true; // 幂等：重复交回视为成功
    }

    bool GameCore::ApplyPossess(uint32_t playerId, const std::string& name, Faction faction, uint32_t tankId)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (playerId == 0 || tankId == 0)
            return false;
        if (static_cast<int>(faction) < 0 || static_cast<int>(faction) >= kFactionCount)
            return false;

        auto tankIt = m_tanks.find(tankId);
        if (tankIt == m_tanks.end())
            return false;

        auto& tank = tankIt->second;
        if (tank->GetFaction() != faction)
            return false;

        if (tank->GetPlayerId() == playerId)
        {
            auto pit = m_players.find(playerId);
            if (pit == m_players.end())
            {
                PlayerInfo player;
                player.id = playerId;
                player.name = name;
                player.faction = faction;
                player.isConnected = true;
                m_players[playerId] = player;
            }
            if (m_nextPlayerId <= playerId)
                m_nextPlayerId = playerId + 1;
            return true;
        }

        if (tank->GetPlayerId() != 0)
            return false;

        PlayerInfo player;
        player.id = playerId;
        player.name = name.empty() ? "Player" : name;
        player.faction = faction;
        player.isConnected = true;
        m_players[playerId] = player;

        tank->SetControlOwner(playerId, true);
        m_aiController.RemoveMemory(tankId);
        if (m_nextPlayerId <= playerId)
            m_nextPlayerId = playerId + 1;
        return true;
    }

    void GameCore::ClearAiMemory()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_aiController.Clear();
    }

    GameLogicSnapshot GameCore::ExportLogicSnapshot() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        GameLogicSnapshot snap;
        snap.frame = m_frame;
        snap.gameState = static_cast<int32_t>(m_state);
        snap.randomSeed = m_rng.GetSeed();
        snap.slotsPerFaction = m_slotsPerFaction;
        snap.nextPlayerId = m_nextPlayerId;
        snap.nextTankId = m_nextTankId;
        snap.nextBulletId = Tank::GetNextBulletId();
        for (int i = 0; i < 4; ++i)
        {
            snap.factionKills[i] = m_factionKills[i];
            snap.factionDeaths[i] = m_factionDeaths[i];
        }

        snap.tanks.reserve(m_tanks.size());
        for (const auto& entry : m_tanks)
            snap.tanks.push_back(entry.second->ExportLogicSnapshot());

        snap.bullets.reserve(m_bullets.size());
        for (const auto& bullet : m_bullets)
        {
            if (!bullet)
                continue;
            BulletLogicSnapshot b;
            b.id = bullet->id;
            b.ownerId = bullet->ownerId;
            b.posX = bullet->position.x;
            b.posY = bullet->position.y;
            b.velX = bullet->velocity.x;
            b.velY = bullet->velocity.y;
            b.damage = bullet->damage;
            b.lifeFrames = bullet->lifeFramesRemaining;
            b.penetrating = bullet->penetrating;
            b.damagedTankIds = bullet->damagedTankIds;
            snap.bullets.push_back(std::move(b));
        }

        snap.players.reserve(m_players.size());
        for (const auto& entry : m_players)
        {
            PlayerLogicSnapshot p;
            p.id = entry.second.id;
            p.name = entry.second.name;
            p.faction = static_cast<int32_t>(entry.second.faction);
            p.kills = entry.second.kills;
            p.score = entry.second.score;
            p.isConnected = entry.second.isConnected;
            snap.players.push_back(std::move(p));
        }

        m_aiController.ExportMemories(snap.aiMemories);
        return snap;
    }

    bool GameCore::ApplyLogicSnapshot(const GameLogicSnapshot& snap)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        m_tanks.clear();
        m_bullets.clear();
        m_players.clear();
        m_frameInputs.clear();
        m_aiController.Clear();

        m_frame = snap.frame;
        m_state = static_cast<GameState>(snap.gameState);
        m_rng.SetSeed(snap.randomSeed == 0 ? 1u : snap.randomSeed);
        m_slotsPerFaction = snap.slotsPerFaction == 0 ? 2u : snap.slotsPerFaction;
        m_nextPlayerId = snap.nextPlayerId == 0 ? 1u : snap.nextPlayerId;
        m_nextTankId = snap.nextTankId == 0 ? 1u : snap.nextTankId;
        Tank::SetNextBulletId(snap.nextBulletId);
        for (int i = 0; i < 4; ++i)
        {
            m_factionKills[i] = snap.factionKills[i];
            m_factionDeaths[i] = snap.factionDeaths[i];
        }

        for (const auto& p : snap.players)
        {
            PlayerInfo info;
            info.id = p.id;
            info.name = p.name;
            info.faction = static_cast<Faction>(p.faction);
            info.kills = p.kills;
            info.score = p.score;
            info.isConnected = p.isConnected;
            m_players[p.id] = info;
        }

        for (const auto& t : snap.tanks)
        {
            FixedVec2 pos{t.posX, t.posY};
            auto tank = std::make_shared<Tank>(
                t.id,
                t.playerId,
                static_cast<Faction>(t.faction),
                static_cast<TankType>(t.type),
                pos,
                t.isPlayer,
                t.rotation);
            tank->ApplyLogicSnapshot(t);
            m_tanks[t.id] = tank;
            if (m_nextTankId <= t.id)
                m_nextTankId = t.id + 1;
        }

        for (const auto& b : snap.bullets)
        {
            auto bullet = std::make_shared<BulletState>();
            bullet->id = b.id;
            bullet->ownerId = b.ownerId;
            bullet->position.x = b.posX;
            bullet->position.y = b.posY;
            bullet->velocity.x = b.velX;
            bullet->velocity.y = b.velY;
            bullet->damage = b.damage;
            bullet->lifeFramesRemaining = b.lifeFrames;
            bullet->penetrating = b.penetrating;
            bullet->damagedTankIds = b.damagedTankIds;
            m_bullets.push_back(bullet);
        }

        m_aiController.ApplyMemories(snap.aiMemories);
        return true;
    }

    std::string GameCore::FormatCompareSnapshot(const char* side) const
    {
        const GameLogicSnapshot snap = ExportLogicSnapshot();
        std::ostringstream oss;
        oss << "[CompareSnap] side=" << (side ? side : "?")
            << " frame=" << snap.frame
            << " state=" << snap.gameState
            << " seed=" << snap.randomSeed
            << " slots=" << snap.slotsPerFaction
            << " nextPlayer=" << snap.nextPlayerId
            << " nextTank=" << snap.nextTankId
            << " nextBullet=" << snap.nextBulletId
            << " tanks=" << snap.tanks.size()
            << " bullets=" << snap.bullets.size()
            << " aiMem=" << snap.aiMemories.size()
            << " kills=" << snap.factionKills[0] << "," << snap.factionKills[1]
            << "," << snap.factionKills[2] << "," << snap.factionKills[3]
            << " deaths=" << snap.factionDeaths[0] << "," << snap.factionDeaths[1]
            << "," << snap.factionDeaths[2] << "," << snap.factionDeaths[3]
            << "\n";

        std::vector<TankLogicSnapshot> tanks = snap.tanks;
        std::sort(tanks.begin(), tanks.end(),
            [](const TankLogicSnapshot& a, const TankLogicSnapshot& b) { return a.id < b.id; });
        for (size_t i = 0; i < tanks.size(); ++i)
        {
            const TankLogicSnapshot& t = tanks[i];
            oss << "  tank id=" << t.id
                << " player=" << t.playerId
                << " fac=" << t.faction
                << " type=" << t.type
                << " Pos=(" << t.posX << "," << t.posY << ")"
                << " world=(" << PosToWorld(t.posX) << "," << PosToWorld(t.posY) << ")"
                << " vel=(" << t.velX << "," << t.velY << ")"
                << " rot=" << t.rotation
                << " turret=" << t.turretRotation
                << " hp=" << t.hp << "/" << t.maxHp
                << " alive=" << (t.isAlive ? 1 : 0)
                << " isPlayer=" << (t.isPlayer ? 1 : 0)
                << " reload=" << t.reloadFrames
                << " lock=" << t.lockedTargetId
                << " aiMode=" << t.aiMoveMode
                << "\n";
        }

        std::vector<BulletLogicSnapshot> bullets = snap.bullets;
        std::sort(bullets.begin(), bullets.end(),
            [](const BulletLogicSnapshot& a, const BulletLogicSnapshot& b) { return a.id < b.id; });
        for (size_t i = 0; i < bullets.size(); ++i)
        {
            const BulletLogicSnapshot& b = bullets[i];
            oss << "  bullet id=" << b.id
                << " owner=" << b.ownerId
                << " Pos=(" << b.posX << "," << b.posY << ")"
                << " vel=(" << b.velX << "," << b.velY << ")"
                << " dmg=" << b.damage
                << " life=" << b.lifeFrames
                << "\n";
        }

        std::vector<AiTankMemorySnapshot> mems = snap.aiMemories;
        std::sort(mems.begin(), mems.end(),
            [](const AiTankMemorySnapshot& a, const AiTankMemorySnapshot& b) {
                return a.tankId < b.tankId;
            });
        for (size_t i = 0; i < mems.size(); ++i)
        {
            const AiTankMemorySnapshot& m = mems[i];
            oss << "  aiMem tank=" << m.tankId
                << " wanderH=" << m.wanderHeading
                << " strafe=" << m.strafeSign
                << " pathGoal=(" << m.pathGoalX << "," << m.pathGoalY << ")"
                << " pathIdx=" << m.pathWaypointIndex
                << " pathN=" << (m.pathWaypointCoords.size() / 2)
                << " target=" << m.pathTargetId
                << " mode=" << m.pathMoveMode
                << "\n";
        }
        return oss.str();
    }
    
    void GameCore::ProcessPlayerInput(const PlayerInput& input)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_players.find(input.playerId) == m_players.end()) return;
        if (!m_players[input.playerId].isConnected) return;
        if (m_state != GameState::Playing) return;

        const uint32_t targetFrame = input.frame != 0 ? input.frame : (m_frame + 1);
        if (targetFrame != m_frame + 1)
            return;

        PlayerInput stored = input;
        stored.frame = targetFrame;
        m_frameInputs[input.playerId] = stored;
    }
    
    void GameCore::StartGame()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        if (m_state != GameState::Waiting) return;

        m_frame = 0;
        m_frameInputs.clear();
        Tank::ResetBulletIdCounter();

        for (const auto& entry : m_players)
        {
            if (!entry.second.isConnected)
                continue;

            uint32_t playerId = entry.first;
            const PlayerInfo& player = entry.second;
            uint32_t tankId = m_nextTankId++;
            auto tank = CreateTankInstance(tankId, playerId, player.faction, true);
            m_tanks[tankId] = tank;
        }
        
        // 按槽位补齐各阵营坦克（不足部分为 AI）
        GenerateAITanks();
        
        m_state = GameState::Playing;
        m_winner = Faction::Soviet;
        m_factionKills[0] = m_factionKills[1] = m_factionKills[2] = m_factionKills[3] = 0;
        m_factionDeaths[0] = m_factionDeaths[1] = m_factionDeaths[2] = m_factionDeaths[3] = 0;
    }

    void GameCore::StartGameAIOnly()
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        if (m_state != GameState::Waiting) return;

        m_frame = 0;
        m_frameInputs.clear();
        m_tanks.clear();
        m_bullets.clear();
        m_aiController.Clear();
        Tank::ResetBulletIdCounter();
        m_nextTankId = 1;

        m_state = GameState::Playing;
        GenerateAITanks();

        m_winner = Faction::Soviet;
        m_factionKills[0] = m_factionKills[1] = m_factionKills[2] = m_factionKills[3] = 0;
        m_factionDeaths[0] = m_factionDeaths[1] = m_factionDeaths[2] = m_factionDeaths[3] = 0;
    }
    
    void GameCore::Reset()
    {
        //std::lock_guard<std::mutex> lock(m_mutex);
        
        m_tanks.clear();
        m_bullets.clear();
        m_players.clear();
        m_frameInputs.clear();
        
        m_frame = 0;
        m_state = GameState::Waiting;
        m_nextPlayerId = 1;
        m_nextTankId = 1;
        Tank::ResetBulletIdCounter();
        m_aiController.Clear();
        m_factionKills[0] = m_factionKills[1] = m_factionKills[2] = m_factionKills[3] = 0;
        m_factionDeaths[0] = m_factionDeaths[1] = m_factionDeaths[2] = m_factionDeaths[3] = 0;
    }
    
    GameSnapshot GameCore::GetGameState() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        GameSnapshot snapshot;
        snapshot.frame = m_frame;
        snapshot.timestamp = LogicFrameToMilliseconds(m_frame);
        snapshot.state = m_state;
        
        for (const auto& entry : m_tanks)
        {
            auto& tank = entry.second;
            snapshot.tanks.push_back(tank->GetState());
        }
        
        for (const auto& bullet : m_bullets)
        {
            snapshot.bullets.push_back(*bullet);
        }
        
        snapshot.obstacles.reserve(m_obstacles.size());
        for (const ObstacleWall& wall : m_obstacles)
            snapshot.obstacles.push_back(ToObstacleWallState(wall));
        
        return snapshot;
    }
    
    std::vector<PlayerInfo> GameCore::GetPlayersInfo() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::vector<PlayerInfo> players;
        for (const auto& entry : m_players)
        {
            auto& player = entry.second;
            players.push_back(player);
        }
        return players;
    }
    
    std::vector<FactionStatus> GameCore::GetFactionStatus() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        
        std::vector<FactionStatus> status;
        status.reserve(kFactionCount);

        for (int f = 0; f < kFactionCount; ++f)
        {
            Faction faction = static_cast<Faction>(f);
            FactionStatus entry;
            entry.faction = faction;
            entry.kills = m_factionKills[f];
            entry.deaths = m_factionDeaths[f];

            for (const auto& pair : m_tanks)
            {
                if (pair.second->GetFaction() != faction)
                    continue;
                entry.totalCount++;
                if (pair.second->IsAlive())
                    entry.aliveCount++;
            }

            status.push_back(entry);
        }
        
        return status;
    }
    
    bool GameCore::IsGameOver() const
    {
        return m_state == GameState::Ended;
    }
    
    Faction GameCore::GetWinner() const
    {
        return m_winner;
    }
    
    void GameCore::SetRandomSeed(uint32_t seed)
    {
        m_rng.SetSeed(seed);
    }

    TankType GameCore::RollTankType(Faction faction, uint32_t entityId, uint32_t salt) const
    {
        switch (faction)
        {
        case Faction::Soviet:
            return static_cast<TankType>(m_rng.UniformInt(m_frame, RngPurpose::TankTypeRoll, entityId, 0, 3, salt));
        case Faction::USA:
            return static_cast<TankType>(3 + m_rng.UniformInt(m_frame, RngPurpose::TankTypeRoll, entityId, 0, 2, salt));
        case Faction::Germany:
            return static_cast<TankType>(5 + m_rng.UniformInt(m_frame, RngPurpose::TankTypeRoll, entityId, 0, 2, salt));
        case Faction::Italy:
            return static_cast<TankType>(7 + m_rng.UniformInt(m_frame, RngPurpose::TankTypeRoll, entityId, 0, 2, salt));
        default:
            return TankType::T34;
        }
    }

    Angle GameCore::RollInitialRotation(uint32_t entityId, RngPurpose purpose, uint32_t salt) const
    {
        return static_cast<Angle>(m_rng.UniformAngleUnits(m_frame, purpose, entityId, salt));
    }

    FixedVec2 GameCore::RollPointInBoundsPos(
        Pos minX,
        Pos maxX,
        Pos minY,
        Pos maxY,
        uint32_t entityId,
        RngPurpose purpose,
        uint32_t salt) const
    {
        const int xSub = m_rng.UniformWorldSubunits(
            m_frame, purpose, entityId, minX, maxX + 1, salt);
        const int ySub = m_rng.UniformWorldSubunits(
            m_frame, purpose, entityId, minY, maxY + 1, salt + 1);
        return FixedVec2(static_cast<Pos>(xSub), static_cast<Pos>(ySub));
    }

    std::shared_ptr<Tank> GameCore::CreateTankInstance(
        uint32_t tankId,
        uint32_t playerId,
        Faction faction,
        bool isPlayer)
    {
        TankType tankType = RollTankType(faction, tankId);
        FixedVec2 spawnPref = GetFactionSpawnPositionPos(faction, tankId);
        FixedVec2 spawnPos = FindUnblockedPosition(
            spawnPref,
            static_cast<Pos>(kNavGridAgentRadiusPosValue),
            faction,
            tankId,
            RngPurpose::SpawnPosition);
        Angle rotation = RollInitialRotation(tankId, RngPurpose::TankInitialRotation);
        auto tank = std::make_shared<Tank>(
            tankId, playerId, faction, tankType, spawnPos, isPlayer, rotation);
        if (!isPlayer)
            tank->SetAI(true);
        return tank;
    }

    bool GameCore::LoadMapObstaclesFromJson(const std::string& json)
    {
        std::lock_guard<std::mutex> lock(m_mutex);

        std::vector<ObstacleWall> loaded;
        Pos worldWidthPos = m_worldWidthPos;
        Pos worldHeightPos = m_worldHeightPos;
        FactionSpawnZone spawnZones[kFactionCount];
        bool hasSpawnZones = false;
        if (!ParseMapConfigFromJson(json, loaded, worldWidthPos, worldHeightPos, spawnZones, hasSpawnZones))
            return false;

        m_obstacles = std::move(loaded);
        m_worldWidthPos = worldWidthPos;
        m_worldHeightPos = worldHeightPos;
        if (hasSpawnZones)
        {
            for (int i = 0; i < kFactionCount; ++i)
                m_spawnZones[i] = spawnZones[i];
            m_hasSpawnZones = true;
        }
        RebuildNavigationGrid();
        return true;
    }

    void GameCore::RebuildNavigationGrid()
    {
        m_navGrid.Build(
            m_worldWidthPos,
            m_worldHeightPos,
            static_cast<Pos>(kNavGridCellSizePosValue),
            m_obstacles,
            static_cast<Pos>(kNavGridAgentRadiusPosValue));
    }
    
    void GameCore::UpdateTanks()
    {
        std::vector<std::shared_ptr<Tank>> allTanks;
        for (const auto& entry : m_tanks)
        {
            auto& tank = entry.second;
            if (tank->IsAlive())
                allTanks.push_back(tank);
        }

        std::vector<std::shared_ptr<Tank>> aiTanks;
        for (const auto& entry : m_tanks)
        {
            auto& tank = entry.second;
            if (tank->IsAlive() && tank->GetPlayerId() == 0)
                aiTanks.push_back(tank);
        }
        std::sort(aiTanks.begin(), aiTanks.end(),
            [](const std::shared_ptr<Tank>& a, const std::shared_ptr<Tank>& b) {
                return a->GetId() < b->GetId();
            });

        for (const auto& tank : aiTanks)
        {
            AiIntent intent = m_aiController.Tick(
                *tank, allTanks, m_obstacles, m_navGrid, m_frame, m_rng);
            tank->ApplyAiIntent(intent);
            if (intent.wantAbility)
                tank->UseAbility();
            if (intent.wantFire)
            {
                auto bullet = tank->Fire();
                if (bullet)
                    m_bullets.push_back(bullet);
            }
        }

        for (const auto& entry : m_tanks)
        {
            auto& tank = entry.second;
            if (tank->IsAlive())
                tank->Update(allTanks);
        }
    }
    
    void GameCore::UpdateBullets()
    {
        const Pos maxPosX = m_worldWidthPos;
        const Pos maxPosY = m_worldHeightPos;

        for (auto it = m_bullets.begin(); it != m_bullets.end();)
        {
            auto& bullet = *it;
            
            bullet->position.x += bullet->velocity.x;
            bullet->position.y += bullet->velocity.y;
            if (bullet->lifeFramesRemaining > 0)
                --bullet->lifeFramesRemaining;
            
            bool hitObstacle = false;
            for (const ObstacleWall& wall : m_obstacles)
            {
                if (PointInsideObstacleFixed(bullet->position.x, bullet->position.y, wall))
                {
                    hitObstacle = true;
                    break;
                }
            }

            if (bullet->position.x < 0 || bullet->position.x > maxPosX ||
                bullet->position.y < 0 || bullet->position.y > maxPosY ||
                bullet->lifeFramesRemaining <= 0 || hitObstacle)
            {
                it = m_bullets.erase(it);
                continue;
            }
            
            ++it;
        }
    }
    
    void GameCore::ClampTankToMap(Tank& tank) const
    {
        const Pos radiusPos = tank.m_config.sizePos;
        const Pos maxPosX = m_worldWidthPos - radiusPos;
        const Pos maxPosY = m_worldHeightPos - radiusPos;

        if (tank.m_position.x < radiusPos)
            tank.m_position.x = radiusPos;
        else if (tank.m_position.x > maxPosX)
            tank.m_position.x = maxPosX;

        if (tank.m_position.y < radiusPos)
            tank.m_position.y = radiusPos;
        else if (tank.m_position.y > maxPosY)
            tank.m_position.y = maxPosY;
    }

    bool GameCore::IsPositionBlocked(const FixedVec2& position, Pos radiusPos) const
    {
        for (const ObstacleWall& wall : m_obstacles)
        {
            if (CircleIntersectsObstacleFixed(position.x, position.y, radiusPos, wall))
                return true;
        }
        return false;
    }

    FixedVec2 GameCore::FindUnblockedPosition(
        const FixedVec2& preferred,
        Pos radiusPos,
        Faction faction,
        uint32_t entityId,
        RngPurpose purpose,
        uint32_t saltBase) const
    {
        if (!IsPositionBlocked(preferred, radiusPos))
            return preferred;

        Pos minX = 0;
        Pos maxX = 0;
        Pos minY = 0;
        Pos maxY = 0;
        GetFactionSpawnBoundsPos(faction, minX, maxX, minY, maxY);

        FixedVec2 best = preferred;
        int64_t bestDistSq = std::numeric_limits<int64_t>::max();
        for (int attempt = 0; attempt < 32; ++attempt)
        {
            FixedVec2 candidate = RollPointInBoundsPos(
                minX, maxX, minY, maxY,
                entityId,
                purpose,
                saltBase + static_cast<uint32_t>(attempt));
            if (IsPositionBlocked(candidate, radiusPos))
                continue;

            const int64_t distSq = FixedDistanceSquared(candidate, preferred);
            if (distSq < bestDistSq)
            {
                bestDistSq = distSq;
                best = candidate;
            }
        }
        return best;
    }

    void GameCore::ResolveObstacleCollisions()
    {
        if (m_obstacles.empty())
            return;

        for (auto& entry : m_tanks)
        {
            Tank* tank = entry.second.get();
            if (!tank->IsAlive())
                continue;

            const Pos radiusPos = TankCollisionRadiusPos(tank->m_config.sizePos);
            for (int iteration = 0; iteration < 3; ++iteration)
            {
                bool moved = false;
                for (const ObstacleWall& wall : m_obstacles)
                {
                    if (ResolveCircleObstacleCollisionFixed(tank->m_position, radiusPos, wall))
                        moved = true;
                }
                if (!moved)
                    break;
            }
            ClampTankToMap(*tank);
        }
    }

    void GameCore::ResolveTankOverlaps()
    {
        std::vector<Tank*> aliveTanks;
        aliveTanks.reserve(m_tanks.size());
        for (auto& entry : m_tanks)
        {
            if (entry.second->IsAlive())
                aliveTanks.push_back(entry.second.get());
        }

        const size_t count = aliveTanks.size();
        if (count < 2)
            return;

        for (int iteration = 0; iteration < kTankOverlapResolveIterations; ++iteration)
        {
            for (size_t i = 0; i < count; ++i)
            {
                Tank* a = aliveTanks[i];
                const Pos radiusAPos = TankCollisionRadiusPos(a->m_config.sizePos);

                for (size_t j = i + 1; j < count; ++j)
                {
                    Tank* b = aliveTanks[j];
                    const Pos radiusBPos = TankCollisionRadiusPos(b->m_config.sizePos);
                    const Pos minDistPos = radiusAPos + radiusBPos;
                    const int64_t minDistSq = static_cast<int64_t>(minDistPos) * minDistPos;

                    const int64_t dx = static_cast<int64_t>(b->m_position.x) - static_cast<int64_t>(a->m_position.x);
                    const int64_t dy = static_cast<int64_t>(b->m_position.y) - static_cast<int64_t>(a->m_position.y);
                    const int64_t distSq = dx * dx + dy * dy;

                    if (distSq >= minDistSq)
                        continue;

                    int32_t dist = 0;
                    int64_t pushX = 0;
                    int64_t pushY = 0;
                    if (distSq > 0)
                    {
                        dist = Isqrt64(distSq);
                        const int64_t overlap = static_cast<int64_t>(minDistPos) - dist;
                        const int64_t push = (overlap + 1) / 2;
                        pushX = (dx * push) / dist;
                        pushY = (dy * push) / dist;
                    }
                    else
                    {
                        const Angle pushAngle = static_cast<Angle>(
                            ((i * 7 + j * 13 + iteration) * kRngAngleUnits) / 360);
                        const int64_t push = static_cast<int64_t>(minDistPos) / 2;
                        pushX = (static_cast<int64_t>(CosQ15(pushAngle)) * push) / kAngleSinCosScale;
                        pushY = (static_cast<int64_t>(SinQ15(pushAngle)) * push) / kAngleSinCosScale;
                    }

                    a->m_position.x -= static_cast<Pos>(pushX);
                    a->m_position.y -= static_cast<Pos>(pushY);
                    b->m_position.x += static_cast<Pos>(pushX);
                    b->m_position.y += static_cast<Pos>(pushY);
                }
            }
        }

        for (Tank* tank : aliveTanks)
            ClampTankToMap(*tank);
    }
    
    void GameCore::CheckCollisions()
    {
        // 子弹与坦克碰撞
        for (auto bulletIt = m_bullets.begin(); bulletIt != m_bullets.end();)
        {
            bool bulletHit = false;
            auto bullet = *bulletIt;
            
            for (auto& entry : m_tanks)
            {
                auto& tank = entry.second;
                if (!tank->IsAlive() || tank->GetId() == bullet->ownerId) continue;
                
                // 查找子弹所有者
                auto ownerIt = m_tanks.find(bullet->ownerId);
                if (ownerIt == m_tanks.end()) continue;
                
                // 计算距离平方（避免 sqrtf）
                const int64_t distSq = FixedDistanceSquared(bullet->position, tank->m_position);
                const Pos hitRadiusPos = tank->m_config.sizePos;
                const int64_t hitRadiusSq = PosDistanceSquared(hitRadiusPos);
                
                if (distSq < hitRadiusSq)
                {
                    // 本方：挡住炮弹，不造成伤害
                    if (!AreHostileFactions(ownerIt->second->GetFaction(), tank->GetFaction()))
                    {
                        bulletHit = true;
                        break;
                    }

                    bool alreadyHit = false;
                    for (uint32_t hitId : bullet->damagedTankIds)
                    {
                        if (hitId == tank->GetId())
                        {
                            alreadyHit = true;
                            break;
                        }
                    }
                    if (alreadyHit)
                        continue;

                    // 应用伤害
                    tank->TakeDamage(bullet->damage, bullet->ownerId);
                    bullet->damagedTankIds.push_back(tank->GetId());
                    
                    // 如果坦克死亡，增加击杀数
                    if (!tank->IsAlive())
                    {
                        auto owner = ownerIt->second;
                        RecordFactionKill(owner->GetFaction(), tank->GetFaction());

                        if (owner->GetPlayerId() != 0)
                        {
                            for (auto& entry1 : m_players)
                            {
                                auto& player = entry1.second;
                                if (player.id == owner->GetPlayerId())
                                {
                                    player.kills++;
                                    player.score += 100;
                                    break;
                                }
                            }
                        }
                    }
                    
                    if (!bullet->penetrating)
                    {
                        bulletHit = true;
                        break;
                    }
                }
            }
            
            if (bulletHit)
            {
                bulletIt = m_bullets.erase(bulletIt);
            }
            else
            {
                ++bulletIt;
            }
        }
    }
    
    void GameCore::GenerateAITanks()
    {
        if (m_state != GameState::Playing) return;

        // 每阵营坦克总数补齐到 m_slotsPerFaction；不足部分刷 AI（已被玩家接管的席位不另加）
        for (int f = 0; f < kFactionCount; f++)
        {
            Faction faction = static_cast<Faction>(f);

            uint32_t factionTotal = 0;
            for (const auto& entry : m_tanks)
            {
                if (entry.second->GetFaction() == faction)
                    ++factionTotal;
            }

            while (factionTotal < m_slotsPerFaction)
            {
                uint32_t tankId = m_nextTankId++;
                auto tank = CreateTankInstance(tankId, 0, faction, false);
                m_tanks[tankId] = tank;
                ++factionTotal;
            }
        }
    }
    
    void GameCore::CleanupDeadUnits()
    {
        for (auto it = m_tanks.begin(); it != m_tanks.end();)
        {
            if (!it->second->IsAlive())
            {
                // 玩家坦克保留在地图中等待复活
                if (it->second->GetPlayerId() != 0)
                {
                    ++it;
                    continue;
                }

                it = m_tanks.erase(it);
            }
            else
            {
                ++it;
            }
        }
    }

    void GameCore::UpdatePlayerRespawns()
    {
        for (auto& entry : m_tanks)
        {
            auto& tank = entry.second;
            if (tank->GetPlayerId() == 0)
                continue;

            if (tank->IsAlive())
                continue;

            if (tank->m_respawnFramesRemaining > 0)
            {
                --tank->m_respawnFramesRemaining;
                if (tank->m_respawnFramesRemaining <= 0)
                {
                    FixedVec2 spawnPos = FindSafeRespawnPosition(tank->GetFaction(), tank->GetId());
                    Angle rotation = RollInitialRotation(
                        tank->GetId(), RngPurpose::TankRespawnRotation);
                    tank->RespawnAt(spawnPos, rotation);
                }
            }
        }
    }

    void GameCore::GetFactionSpawnBoundsPos(Faction faction, Pos& minX, Pos& maxX, Pos& minY, Pos& maxY) const
    {
        int idx = FactionIndex(faction);
        if (m_hasSpawnZones && idx >= 0 && idx < kFactionCount)
        {
            const FactionSpawnZone& zone = m_spawnZones[idx];
            minX = zone.minX;
            maxX = zone.maxX;
            minY = zone.minY;
            maxY = zone.maxY;
            return;
        }

        const Pos margin = static_cast<Pos>(kFactionSpawnMarginPosValue);
        const Pos spread = static_cast<Pos>(kFactionSpawnSpreadPosValue);

        switch (faction)
        {
        case Faction::Soviet:
            minX = margin;
            maxX = margin + spread;
            minY = margin;
            maxY = margin + spread;
            break;
        case Faction::USA:
            minX = m_worldWidthPos - margin - spread;
            maxX = m_worldWidthPos - margin;
            minY = margin;
            maxY = margin + spread;
            break;
        case Faction::Germany:
            minX = margin;
            maxX = margin + spread;
            minY = m_worldHeightPos - margin - spread;
            maxY = m_worldHeightPos - margin;
            break;
        case Faction::Italy:
            minX = m_worldWidthPos - margin - spread;
            maxX = m_worldWidthPos - margin;
            minY = m_worldHeightPos - margin - spread;
            maxY = m_worldHeightPos - margin;
            break;
        default:
            minX = m_worldWidthPos / 2 - spread / 2;
            maxX = m_worldWidthPos / 2 + spread / 2;
            minY = m_worldHeightPos / 2 - spread / 2;
            maxY = m_worldHeightPos / 2 + spread / 2;
            break;
        }
    }

    FixedVec2 GameCore::FindSafeRespawnPosition(Faction faction, uint32_t entityId) const
    {
        Pos minX = 0;
        Pos maxX = 0;
        Pos minY = 0;
        Pos maxY = 0;
        GetFactionSpawnBoundsPos(faction, minX, maxX, minY, maxY);

        const Pos radiusPos = static_cast<Pos>(kNavGridAgentRadiusPosValue);
        FixedVec2 bestFixed = RollPointInBoundsPos(
            minX, maxX, minY, maxY, entityId, RngPurpose::RespawnPosition, 0);
        bestFixed = FindUnblockedPosition(
            bestFixed, radiusPos, faction, entityId, RngPurpose::UnblockedPosition, 0);
        int64_t bestMinDistSq = -1;
        const int64_t minEnemyDistSq =
            PosDistanceSquared(static_cast<Pos>(kMinRespawnEnemyDistancePosValue));

        for (int attempt = 0; attempt < 24; ++attempt)
        {
            FixedVec2 posFixed = RollPointInBoundsPos(
                minX, maxX, minY, maxY, entityId, RngPurpose::RespawnPosition, static_cast<uint32_t>(attempt + 1));
            posFixed = FindUnblockedPosition(
                posFixed,
                radiusPos,
                faction,
                entityId,
                RngPurpose::UnblockedPosition,
                static_cast<uint32_t>((attempt + 1) * 32));
            int64_t minEnemyDistSqLocal = std::numeric_limits<int64_t>::max();

            for (const auto& entry : m_tanks)
            {
                const auto& other = entry.second;
                if (!other->IsAlive()) continue;
                if (!AreHostileFactions(faction, other->GetFaction())) continue;

                const int64_t distSq = FixedDistanceSquared(posFixed, other->m_position);
                if (distSq < minEnemyDistSqLocal)
                    minEnemyDistSqLocal = distSq;
            }

            if (minEnemyDistSqLocal >= minEnemyDistSq)
                return posFixed;

            if (minEnemyDistSqLocal > bestMinDistSq)
            {
                bestMinDistSq = minEnemyDistSqLocal;
                bestFixed = posFixed;
            }
        }

        return bestFixed;
    }

    FixedVec2 GameCore::GetFactionSpawnPositionPos(Faction faction, uint32_t entityId, uint32_t salt) const
    {
        Pos minX = 0;
        Pos maxX = 0;
        Pos minY = 0;
        Pos maxY = 0;
        GetFactionSpawnBoundsPos(faction, minX, maxX, minY, maxY);
        return RollPointInBoundsPos(
            minX, maxX, minY, maxY, entityId, RngPurpose::FactionSpawnPosition, salt);
    }
    
    int GameCore::FactionIndex(Faction faction)
    {
        switch (faction)
        {
        case Faction::Soviet: return 0;
        case Faction::USA: return 1;
        case Faction::Germany: return 2;
        case Faction::Italy: return 3;
        default: return -1;
        }
    }

    void GameCore::RecordFactionKill(Faction killerFaction, Faction victimFaction)
    {
        int killerIdx = FactionIndex(killerFaction);
        int victimIdx = FactionIndex(victimFaction);
        if (killerIdx >= 0 && victimIdx >= 0 && killerIdx != victimIdx)
            m_factionKills[killerIdx]++;
        if (victimIdx >= 0)
            m_factionDeaths[victimIdx]++;
    }

    Faction GameCore::ResolveWinnerByBattleStats() const
    {
        Faction best = Faction::Soviet;
        uint32_t bestKills = 0;
        uint32_t bestDeaths = std::numeric_limits<uint32_t>::max();
        bool hasCandidate = false;

        for (int f = 0; f < kFactionCount; ++f)
        {
            uint32_t kills = m_factionKills[f];
            uint32_t deaths = m_factionDeaths[f];
            if (!hasCandidate
                || kills > bestKills
                || (kills == bestKills && deaths < bestDeaths))
            {
                hasCandidate = true;
                bestKills = kills;
                bestDeaths = deaths;
                best = static_cast<Faction>(f);
            }
        }

        return best;
    }

    void GameCore::CheckGameEnd()
    {
        // 无限复活/补员：不在此做歼灭判定；对局结束时按杀敌数与己方死亡数结算（见 ResolveWinnerByBattleStats）
    }
}
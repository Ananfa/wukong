#include "TankBattleWrapper.h"
#include "../Core/GameCore.h"
#include "../Common/Constants.h"
#include "../Common/FixedMath.h"
#include <cstring>
#include <vector>

using namespace TankBattle;

// C++游戏核心的包装器
class GameCoreWrapper
{
public:
    GameCore gameCore;
    GameLogicSnapshot pendingLogicSnap;
    bool pendingLogicSnapActive = false;
};

extern "C"
{
    TANKBATTLE_API TankBattleGame TB_CreateGame()
    {
        return new GameCoreWrapper();
    }
    
    TANKBATTLE_API void TB_DestroyGame(TankBattleGame game)
    {
        delete static_cast<GameCoreWrapper*>(game);
    }
    
    TANKBATTLE_API unsigned char TB_Initialize(TankBattleGame game, unsigned int maxPlayers)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.Initialize(maxPlayers) ? 1 : 0;
        //return 0;
    }
    
    TANKBATTLE_API void TB_Update(TankBattleGame game, float /*deltaTime*/)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.AdvanceSimulation();
    }

    TANKBATTLE_API unsigned int TB_GetFrame(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.GetFrame();
    }

    TANKBATTLE_API unsigned char TB_SetFrameInputs(
        TankBattleGame game,
        unsigned int frame,
        const TB_PlayerInput* inputs,
        unsigned int count)
    {
        if (!game)
            return 0;

        auto wrapper = static_cast<GameCoreWrapper*>(game);
        if (count == 0 || inputs == nullptr)
            return wrapper->gameCore.SetFrameInputs(frame, nullptr, 0) ? 1 : 0;

        std::vector<PlayerInput> nativeInputs;
        nativeInputs.reserve(count);
        for (unsigned int i = 0; i < count; ++i)
        {
            const TB_PlayerInput& src = inputs[i];
            PlayerInput dst;
            dst.playerId = src.playerId;
            dst.frame = src.frame;
            dst.moveX = src.moveX;
            dst.moveY = src.moveY;
            dst.aimX = src.aimX;
            dst.aimY = src.aimY;
            dst.fire = src.fire != 0;
            dst.useAbility = src.useAbility != 0;
            dst.timestamp = src.timestamp;
            nativeInputs.push_back(dst);
        }

        return wrapper->gameCore.SetFrameInputs(frame, nativeInputs.data(), nativeInputs.size()) ? 1 : 0;
    }

    TANKBATTLE_API void TB_AdvanceSimulation(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.AdvanceSimulation();
    }

    TANKBATTLE_API float TB_GetFixedLogicDeltaTime()
    {
        return kFixedLogicDeltaTime;
    }
    
    TANKBATTLE_API unsigned int TB_AddPlayer(TankBattleGame game, const char* name, int faction)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.AddPlayer(name, static_cast<Faction>(faction));
    }
    
    TANKBATTLE_API void TB_RemovePlayer(TankBattleGame game, unsigned int playerId)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.RemovePlayer(playerId);
    }

    TANKBATTLE_API void TB_SetSlotsPerFaction(TankBattleGame game, unsigned int slots)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.SetSlotsPerFaction(slots);
    }

    TANKBATTLE_API unsigned int TB_GetSlotsPerFaction(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.GetSlotsPerFaction();
    }

    TANKBATTLE_API unsigned int TB_CountFreeAISlots(TankBattleGame game, int faction)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.CountFreeAISlots(static_cast<Faction>(faction));
    }

    TANKBATTLE_API unsigned int TB_PossessAITank(
        TankBattleGame game,
        const char* name,
        int faction,
        unsigned int* outTankId)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.PossessAITank(
            name ? name : "Player",
            static_cast<Faction>(faction),
            outTankId);
    }

    TANKBATTLE_API unsigned char TB_ReleaseToAI(TankBattleGame game, unsigned int playerId)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.ReleaseToAI(playerId) ? 1 : 0;
    }

    TANKBATTLE_API unsigned char TB_ApplyPossess(
        TankBattleGame game,
        unsigned int playerId,
        const char* name,
        int faction,
        unsigned int tankId)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.ApplyPossess(
            playerId,
            name ? name : "Player",
            static_cast<Faction>(faction),
            tankId) ? 1 : 0;
    }

    TANKBATTLE_API unsigned char TB_ApplyRelease(TankBattleGame game, unsigned int playerId)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.ApplyRelease(playerId) ? 1 : 0;
    }

    TANKBATTLE_API void TB_ClearAiMemory(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.ClearAiMemory();
    }

    TANKBATTLE_API int TB_FormatCompareSnapshot(
        TankBattleGame game,
        const char* side,
        char* buf,
        int bufSize)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        const std::string text = wrapper->gameCore.FormatCompareSnapshot(side);
        if (!buf || bufSize <= 0)
            return static_cast<int>(text.size());
        const int n = static_cast<int>(text.size());
        const int copy = n < (bufSize - 1) ? n : (bufSize - 1);
        if (copy > 0)
            memcpy(buf, text.data(), static_cast<size_t>(copy));
        buf[copy] = '\0';
        return n;
    }

    static void FillHeader(GameLogicSnapshot& snap, const TB_LogicSnapshotHeader* header)
    {
        snap.frame = header->frame;
        snap.gameState = header->gameState;
        snap.randomSeed = header->randomSeed;
        snap.slotsPerFaction = header->slotsPerFaction;
        snap.nextPlayerId = header->nextPlayerId;
        snap.nextTankId = header->nextTankId;
        snap.nextBulletId = header->nextBulletId;
        for (int i = 0; i < 4; ++i)
        {
            snap.factionKills[i] = header->factionKills[i];
            snap.factionDeaths[i] = header->factionDeaths[i];
        }
    }

    static TankLogicSnapshot ToTankLogic(const TB_TankLogicSnapshot& src)
    {
        TankLogicSnapshot t;
        t.id = src.id;
        t.playerId = src.playerId;
        t.faction = src.faction;
        t.type = src.type;
        t.posX = src.posX;
        t.posY = src.posY;
        t.velX = src.velX;
        t.velY = src.velY;
        t.rotation = src.rotation;
        t.turretRotation = src.turretRotation;
        t.hp = src.hp;
        t.maxHp = src.maxHp;
        t.shieldFrames = src.shieldFrames;
        t.speedBoostFrames = src.speedBoostFrames;
        t.rapidFireFrames = src.rapidFireFrames;
        t.abilityCooldownFrames = src.abilityCooldownFrames;
        t.reloadFrames = src.reloadFrames;
        t.reloadDurationFrames = src.reloadDurationFrames;
        t.recoilVelX = src.recoilVelX;
        t.recoilVelY = src.recoilVelY;
        t.lockedTargetId = src.lockedTargetId;
        t.aiMoveMode = src.aiMoveMode;
        t.respawnFrames = src.respawnFrames;
        t.spawnProtectionFrames = src.spawnProtectionFrames;
        t.chargedShot = src.chargedShot != 0;
        t.isPlayer = src.isPlayer != 0;
        t.isAlive = src.isAlive != 0;
        return t;
    }

    TANKBATTLE_API void TB_LogicSnap_Begin(TankBattleGame game, const TB_LogicSnapshotHeader* header)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->pendingLogicSnap = GameLogicSnapshot();
        wrapper->pendingLogicSnapActive = false;
        if (!header)
            return;
        FillHeader(wrapper->pendingLogicSnap, header);
        wrapper->pendingLogicSnapActive = true;
    }

    TANKBATTLE_API void TB_LogicSnap_AddTank(TankBattleGame game, const TB_TankLogicSnapshot* tank)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        if (!wrapper->pendingLogicSnapActive || !tank)
            return;
        wrapper->pendingLogicSnap.tanks.push_back(ToTankLogic(*tank));
    }

    TANKBATTLE_API void TB_LogicSnap_AddBullet(TankBattleGame game, const TB_BulletLogicSnapshot* bullet)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        if (!wrapper->pendingLogicSnapActive || !bullet)
            return;
        BulletLogicSnapshot b;
        b.id = bullet->id;
        b.ownerId = bullet->ownerId;
        b.posX = bullet->posX;
        b.posY = bullet->posY;
        b.velX = bullet->velX;
        b.velY = bullet->velY;
        b.damage = bullet->damage;
        b.lifeFrames = bullet->lifeFrames;
        b.penetrating = bullet->penetrating != 0;
        const unsigned int n = bullet->damagedCount > 8u ? 8u : bullet->damagedCount;
        b.damagedTankIds.reserve(n);
        for (unsigned int d = 0; d < n; ++d)
            b.damagedTankIds.push_back(bullet->damagedTankIds[d]);
        wrapper->pendingLogicSnap.bullets.push_back(std::move(b));
    }

    TANKBATTLE_API void TB_LogicSnap_AddPlayer(
        TankBattleGame game,
        unsigned int id,
        const char* name,
        int faction,
        unsigned int kills,
        unsigned int score,
        unsigned char isConnected)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        if (!wrapper->pendingLogicSnapActive)
            return;
        PlayerLogicSnapshot p;
        p.id = id;
        p.name = name ? name : "";
        p.faction = faction;
        p.kills = kills;
        p.score = score;
        p.isConnected = isConnected != 0;
        wrapper->pendingLogicSnap.players.push_back(std::move(p));
    }

    TANKBATTLE_API void TB_LogicSnap_AddAiMemory(TankBattleGame game, const TB_AiMemorySnapshot* memory)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        if (!wrapper->pendingLogicSnapActive || !memory)
            return;
        AiTankMemorySnapshot m;
        m.tankId = memory->tankId;
        m.wanderHeading = memory->wanderHeading;
        m.strafeSign = memory->strafeSign;
        m.strafeSwitchFrames = memory->strafeSwitchFrames;
        m.wanderGoalSerial = memory->wanderGoalSerial;
        m.pathGoalX = memory->pathGoalX;
        m.pathGoalY = memory->pathGoalY;
        m.pathTargetId = memory->pathTargetId;
        m.pathMoveMode = memory->pathMoveMode;
        m.pathRecalcFrames = memory->pathRecalcFrames;
        m.wanderPathGoalX = memory->wanderPathGoalX;
        m.wanderPathGoalY = memory->wanderPathGoalY;
        m.wanderPathFrames = memory->wanderPathFrames;
        m.pathWaypointIndex = memory->pathWaypointIndex;
        unsigned int n = memory->waypointCoordCount;
        if (n > 64u)
            n = 64u;
        m.pathWaypointCoords.reserve(n);
        for (unsigned int i = 0; i < n; ++i)
            m.pathWaypointCoords.push_back(memory->pathWaypointCoords[i]);
        wrapper->pendingLogicSnap.aiMemories.push_back(std::move(m));
    }

    TANKBATTLE_API unsigned char TB_LogicSnap_Commit(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        if (!wrapper->pendingLogicSnapActive)
            return 0;
        const bool ok = wrapper->gameCore.ApplyLogicSnapshot(wrapper->pendingLogicSnap);
        wrapper->pendingLogicSnapActive = false;
        wrapper->pendingLogicSnap = GameLogicSnapshot();
        return ok ? 1 : 0;
    }

    TANKBATTLE_API unsigned int TB_LogicSnap_AiMemoryStructSize()
    {
        return static_cast<unsigned int>(sizeof(TB_AiMemorySnapshot));
    }

    TANKBATTLE_API unsigned int TB_LogicSnap_TankStructSize()
    {
        return static_cast<unsigned int>(sizeof(TB_TankLogicSnapshot));
    }

    TANKBATTLE_API unsigned int TB_LogicSnap_BulletStructSize()
    {
        return static_cast<unsigned int>(sizeof(TB_BulletLogicSnapshot));
    }

    TANKBATTLE_API unsigned int TB_LogicSnap_HeaderStructSize()
    {
        return static_cast<unsigned int>(sizeof(TB_LogicSnapshotHeader));
    }
    
    TANKBATTLE_API void TB_ProcessPlayerInput(TankBattleGame game, const TB_PlayerInput* input)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        
        PlayerInput pi;
        pi.playerId = input->playerId;
        pi.frame = input->frame;
        pi.moveX = input->moveX;
        pi.moveY = input->moveY;
        pi.aimX = input->aimX;
        pi.aimY = input->aimY;
        pi.fire = input->fire != 0;
        pi.useAbility = input->useAbility != 0;
        pi.timestamp = input->timestamp;
        
        wrapper->gameCore.ProcessPlayerInput(pi);
    }
    
    TANKBATTLE_API void TB_StartGame(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.StartGame();
    }

    TANKBATTLE_API void TB_StartGameAIOnly(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.StartGameAIOnly();
    }
    
    TANKBATTLE_API void TB_Reset(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.Reset();
    }
    
    TANKBATTLE_API TB_GameSnapshot* TB_GetGameState(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        GameSnapshot snapshot = wrapper->gameCore.GetGameState();
        
        // 分配内存
        auto* tbSnapshot = new TB_GameSnapshot;
        tbSnapshot->frame = snapshot.frame;
        tbSnapshot->timestamp = snapshot.timestamp;
        tbSnapshot->state = static_cast<int>(snapshot.state);
        
        // 复制坦克数据
        tbSnapshot->tankCount = static_cast<unsigned int>(snapshot.tanks.size());
        tbSnapshot->tanks = new TB_TankState[tbSnapshot->tankCount];
        for (size_t i = 0; i < snapshot.tanks.size(); i++)
        {
            const auto& src = snapshot.tanks[i];
            auto& dst = tbSnapshot->tanks[i];
            
            dst.id = src.id;
            dst.playerId = src.playerId;
            dst.faction = static_cast<int>(src.faction);
            dst.type = static_cast<int>(src.type);
            dst.position = {src.position.x, src.position.y};
            dst.velocity = {src.velocity.x, src.velocity.y};
            dst.rotation = src.rotation;
            dst.turretRotation = src.turretRotation;
            dst.hp = src.hp;
            dst.maxHp = src.maxHp;
            dst.shield = src.shield;
            dst.speedBoost = src.speedBoost;
            dst.rapidFire = src.rapidFire;
            dst.abilityCooldown = src.abilityCooldown;
            dst.isAlive = src.isAlive ? 1 : 0;
            dst.lockedTargetId = src.lockedTargetId;
            dst.aiMoveMode = src.aiMoveMode;
            dst.moveSpeed = src.moveSpeed;
            dst.respawnTimeRemaining = src.respawnTimeRemaining;
            dst.reloadTimeRemaining = src.reloadTimeRemaining;
            dst.reloadDuration = src.reloadDuration;
        }
        
        // 复制子弹数据
        tbSnapshot->bulletCount = static_cast<unsigned int>(snapshot.bullets.size());
        tbSnapshot->bullets = new TB_BulletState[tbSnapshot->bulletCount];
        for (size_t i = 0; i < snapshot.bullets.size(); i++)
        {
            const auto& src = snapshot.bullets[i];
            auto& dst = tbSnapshot->bullets[i];
            
            dst.id = src.id;
            dst.ownerId = src.ownerId;
            dst.position = {PosToWorld(src.position.x), PosToWorld(src.position.y)};
            dst.velocity = {
                VelPerFrameToWorldSpeed(src.velocity.x),
                VelPerFrameToWorldSpeed(src.velocity.y)
            };
            dst.damage = static_cast<float>(src.damage);
            dst.lifeTime = TankBattle::LogicFramesToSeconds(src.lifeFramesRemaining);
            dst.penetrating = src.penetrating ? 1 : 0;
        }
        
        tbSnapshot->obstacleCount = static_cast<unsigned int>(snapshot.obstacles.size());
        tbSnapshot->obstacles = new TB_ObstacleWallState[tbSnapshot->obstacleCount];
        for (size_t i = 0; i < snapshot.obstacles.size(); i++)
        {
            const auto& src = snapshot.obstacles[i];
            auto& dst = tbSnapshot->obstacles[i];
            dst.centerX = src.centerX;
            dst.centerY = src.centerY;
            dst.width = src.width;
            dst.height = src.height;
            dst.rotation = src.rotation;
        }
        
        return tbSnapshot;
    }
    
    TANKBATTLE_API void TB_FreeGameState(TB_GameSnapshot* snapshot)
    {
        if (!snapshot) return;
        
        delete[] snapshot->tanks;
        delete[] snapshot->bullets;
        delete[] snapshot->obstacles;
        delete snapshot;
    }
    
    TANKBATTLE_API TB_PlayerInfo* TB_GetPlayersInfo(TankBattleGame game, unsigned int* count)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        auto players = wrapper->gameCore.GetPlayersInfo();
        
        *count = static_cast<unsigned int>(players.size());
        auto* tbPlayers = new TB_PlayerInfo[players.size()];
        
        for (size_t i = 0; i < players.size(); i++)
        {
            const auto& src = players[i];
            auto& dst = tbPlayers[i];
            
            dst.id = src.id;
            dst.name = _strdup(src.name.c_str()); // 需要调用者释放
            dst.faction = static_cast<int>(src.faction);
            dst.kills = src.kills;
            dst.score = src.score;
            dst.isConnected = src.isConnected ? 1 : 0;
        }
        
        return tbPlayers;
    }
    
    TANKBATTLE_API void TB_FreePlayersInfo(TB_PlayerInfo* players, unsigned int count)
    {
        if (!players) return;
        
        for (unsigned int i = 0; i < count; i++)
        {
            free(const_cast<char*>(players[i].name));
        }
        
        delete[] players;
    }
    
    TANKBATTLE_API TB_FactionStatus* TB_GetFactionStatus(TankBattleGame game, unsigned int* count)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        auto status = wrapper->gameCore.GetFactionStatus();
        
        *count = static_cast<unsigned int>(status.size());
        auto* tbStatus = new TB_FactionStatus[status.size()];
        
        for (size_t i = 0; i < status.size(); i++)
        {
            const auto& src = status[i];
            auto& dst = tbStatus[i];
            
            dst.faction = static_cast<int>(src.faction);
            dst.aliveCount = src.aliveCount;
            dst.totalCount = src.totalCount;
            dst.kills = src.kills;
            dst.deaths = src.deaths;
        }
        
        return tbStatus;
    }
    
    TANKBATTLE_API void TB_FreeFactionStatus(TB_FactionStatus* status, unsigned int count)
    {
        delete[] status;
    }
    
    TANKBATTLE_API unsigned char TB_IsGameOver(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.IsGameOver() ? 1 : 0;
    }
    
    TANKBATTLE_API int TB_GetWinner(TankBattleGame game)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return static_cast<int>(wrapper->gameCore.GetWinner());
    }
    
    TANKBATTLE_API void TB_SetRandomSeed(TankBattleGame game, unsigned int seed)
    {
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        wrapper->gameCore.SetRandomSeed(seed);
    }

    TANKBATTLE_API unsigned char TB_LoadMapObstaclesJson(TankBattleGame game, const char* json)
    {
        if (!game || !json)
            return 0;
        auto wrapper = static_cast<GameCoreWrapper*>(game);
        return wrapper->gameCore.LoadMapObstaclesFromJson(json) ? 1 : 0;
    }
}
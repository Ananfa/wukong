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
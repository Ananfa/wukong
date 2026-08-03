#pragma once
#include "../Common/Types.h"
#include "../Common/Constants.h"
#include "../Common/GameRng.h"
#include "Tank.h"
#include "ObstacleWall.h"
#include "Ai/AiController.h"
#include "Ai/NavigationGrid.h"
#include <vector>
#include <memory>
#include <map>
#include <mutex>
#include <string>

namespace TankBattle
{
    class GameCore
    {
    public:
        GameCore();
        ~GameCore();
        
        // 初始化游戏
        bool Initialize(uint32_t maxPlayers = 8);
        
        // 更新游戏逻辑（固定 30Hz，每帧一步；等价于 AdvanceSimulation，需先 SetFrameInputs）
        void Update();

        uint32_t GetFrame() const;

        // 设置下一逻辑帧的玩家输入（frame 必须等于 GetFrame()+1；无输入的玩家本帧不移动/不开火）
        bool SetFrameInputs(uint32_t frame, const PlayerInput* inputs, size_t count);

        // 推进一帧模拟（应用已设置的帧输入后执行物理/AI/碰撞）
        void AdvanceSimulation();
        
        // 添加玩家（Waiting：仅注册；Playing：仍会新建坦克，兼容旧本地流程）
        uint32_t AddPlayer(const std::string& name, Faction faction);
        
        // 移除玩家（删除其坦克；联网交回 AI 请用 ReleaseToAI）
        void RemovePlayer(uint32_t playerId);

        // 每阵营坦克槽位数（开局 AI 名额 / 可被接管席位），默认 2
        void SetSlotsPerFaction(uint32_t slots);
        uint32_t GetSlotsPerFaction() const;

        // 指定阵营当前可被接管的 AI 坦克数（含尚未刷出的空槽）
        uint32_t CountFreeAISlots(Faction faction) const;

        // 注册玩家并接管该阵营一台 AI 坦克；失败返回 0。outTankId 可选。
        uint32_t PossessAITank(const std::string& name, Faction faction, uint32_t* outTankId = nullptr);

        // 将玩家控制权交回 AI（坦克保留）；成功返回 true
        bool ReleaseToAI(uint32_t playerId);

        // 按指定 playerId/tankId 接管（幂等，供 FrameSync 事件 / 全量快照对齐）
        bool ApplyPossess(uint32_t playerId, const std::string& name, Faction faction, uint32_t tankId);

        // 与 ReleaseToAI 相同，供控制事件命名
        bool ApplyRelease(uint32_t playerId) { return ReleaseToAI(playerId); }

        // 导出 / 应用定点全量逻辑状态（中途加入）
        GameLogicSnapshot ExportLogicSnapshot() const;
        bool ApplyLogicSnapshot(const GameLogicSnapshot& snap);

        // 同步对比用：格式化 ExportLogicSnapshot（坦克按 id 排序）
        std::string FormatCompareSnapshot(const char* side) const;

        // 清空 AI 记忆（全量同步点：服与端对齐）
        void ClearAiMemory();
        
        // 处理玩家输入
        void ProcessPlayerInput(const PlayerInput& input);
        
        // 开始游戏：为已注册玩家造车，再按槽位补齐各阵营 AI
        void StartGame();

        // 仅按槽位生成各阵营 AI（无人造车）；供「接管 AI」模式开局
        void StartGameAIOnly();
        
        // 重置游戏
        void Reset();
        
        // 获取游戏状态
        GameSnapshot GetGameState() const;
        
        // 获取玩家信息
        std::vector<PlayerInfo> GetPlayersInfo() const;
        
        // 获取阵营状态
        std::vector<FactionStatus> GetFactionStatus() const;
        
        // 检查游戏是否结束
        bool IsGameOver() const;
        
        // 获取获胜阵营
        Faction GetWinner() const;
        
        // 设置随机种子（用于确定性更新）
        void SetRandomSeed(uint32_t seed);

        // 从 JSON 加载地图阻挡墙（Unity 传入 Assets/Config/MapObstacles.json 文本）
        bool LoadMapObstaclesFromJson(const std::string& json);

        void RebuildNavigationGrid();
        
    private:
        // 更新坦克
        void UpdateTanks();
        
        void UpdateBullets();
        
        // 检测碰撞
        void CheckCollisions();

        // 坦克圆形互阻：重叠时沿分离方向推开
        void ResolveTankOverlaps();

        void ClampTankToMap(Tank& tank) const;

        void ResolveObstacleCollisions();

        bool IsPositionBlocked(const FixedVec2& position, Pos radiusPos) const;

        FixedVec2 FindUnblockedPosition(
            const FixedVec2& preferred,
            Pos radiusPos,
            Faction faction,
            uint32_t entityId,
            RngPurpose purpose,
            uint32_t saltBase = 0) const;
        
        // 生成AI坦克
        void GenerateAITanks();
        
        // 清理死亡单位
        void CleanupDeadUnits();

        // 玩家死亡后复活
        void UpdatePlayerRespawns();

        TankType RollTankType(Faction faction, uint32_t entityId, uint32_t salt = 0) const;

        Angle RollInitialRotation(uint32_t entityId, RngPurpose purpose, uint32_t salt = 0) const;

        FixedVec2 RollPointInBoundsPos(
            Pos minX,
            Pos maxX,
            Pos minY,
            Pos maxY,
            uint32_t entityId,
            RngPurpose purpose,
            uint32_t salt) const;

        std::shared_ptr<Tank> CreateTankInstance(
            uint32_t tankId,
            uint32_t playerId,
            Faction faction,
            bool isPlayer);

        void RecordFactionKill(Faction killerFaction, Faction victimFaction);

        static int FactionIndex(Faction faction);

        FixedVec2 FindSafeRespawnPosition(Faction faction, uint32_t entityId) const;

        FixedVec2 GetFactionSpawnPositionPos(Faction faction, uint32_t entityId, uint32_t salt = 0) const;

        void GetFactionSpawnBoundsPos(Faction faction, Pos& minX, Pos& maxX, Pos& minY, Pos& maxY) const;

        Faction ResolveWinnerByBattleStats() const;

        // 检查游戏结束条件
        void CheckGameEnd();

        void ApplyPlayerFrameInputs();
        
    private:
        mutable std::mutex m_mutex;
        uint32_t m_frame = 0;
        GameState m_state = GameState::Waiting;
        
        std::map<uint32_t, std::shared_ptr<Tank>> m_tanks;
        std::vector<std::shared_ptr<BulletState>> m_bullets;
        
        std::map<uint32_t, PlayerInfo> m_players;
        std::map<uint32_t, PlayerInput> m_frameInputs;
        
        uint32_t m_nextPlayerId = 1;
        uint32_t m_nextTankId = 1;
        GameRng m_rng;
        
        Pos m_worldWidthPos = static_cast<Pos>(kDefaultWorldSizePosValue);
        Pos m_worldHeightPos = static_cast<Pos>(kDefaultWorldSizePosValue);

        std::vector<ObstacleWall> m_obstacles;
        NavigationGrid m_navGrid;
        FactionSpawnZone m_spawnZones[kFactionCount];
        bool m_hasSpawnZones = false;
        
        Faction m_winner = Faction::Soviet;
        AiController m_aiController;
        uint32_t m_factionKills[4] = {0, 0, 0, 0};
        uint32_t m_factionDeaths[4] = {0, 0, 0, 0};
        uint32_t m_slotsPerFaction = 2; // 配置项，Reset 不清除
    };
}
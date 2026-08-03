#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// 游戏核心句柄
typedef void* TankBattleGame;

// 向量结构
typedef struct
{
    float x;
    float y;
} TB_Vector2;

// 坦克状态
typedef struct
{
    unsigned int id;
    unsigned int playerId;
    int faction;
    int type;
    TB_Vector2 position;
    TB_Vector2 velocity;
    float rotation;
    float turretRotation;
    float hp;
    float maxHp;
    float shield;
    float speedBoost;
    float rapidFire;
    float abilityCooldown;
    unsigned char isAlive;
    unsigned int lockedTargetId;
    unsigned int aiMoveMode;
    float moveSpeed;
    float respawnTimeRemaining;
    float reloadTimeRemaining;
    float reloadDuration;
} TB_TankState;

// 子弹状态
typedef struct
{
    unsigned int id;
    unsigned int ownerId;
    TB_Vector2 position;
    TB_Vector2 velocity;
    float damage;
    float lifeTime;
    unsigned char penetrating;
} TB_BulletState;

// 阻挡墙状态
typedef struct
{
    float centerX;
    float centerY;
    float width;
    float height;
    float rotation;
} TB_ObstacleWallState;

// 游戏快照
typedef struct
{
    unsigned int frame;
    unsigned long long timestamp;
    TB_TankState* tanks;
    unsigned int tankCount;
    TB_BulletState* bullets;
    unsigned int bulletCount;
    TB_ObstacleWallState* obstacles;
    unsigned int obstacleCount;
    int state;
} TB_GameSnapshot;

// 玩家信息
typedef struct
{
    unsigned int id;
    const char* name;
    int faction;
    unsigned int kills;
    unsigned int score;
    unsigned char isConnected;
} TB_PlayerInfo;

// 阵营状态
typedef struct
{
    int faction;
    unsigned int aliveCount;
    unsigned int totalCount;
    unsigned int kills;
    unsigned int deaths;
} TB_FactionStatus;

// 玩家输入（Q15 方向 int16，与 Core PlayerInput 布局一致）
#pragma pack(push, 1)
typedef struct
{
    unsigned int playerId;
    unsigned int frame;
    short moveX;
    short moveY;
    short aimX;
    short aimY;
    unsigned char fire;
    unsigned char useAbility;
    unsigned long long timestamp;
} TB_PlayerInput;
#pragma pack(pop)

#ifdef _WIN32
#define TANKBATTLE_API __declspec(dllexport)
#else
#define TANKBATTLE_API
#endif

// 创建游戏实例
TANKBATTLE_API TankBattleGame TB_CreateGame();

// 销毁游戏实例
TANKBATTLE_API void TB_DestroyGame(TankBattleGame game);

// 初始化游戏
TANKBATTLE_API unsigned char TB_Initialize(TankBattleGame game, unsigned int maxPlayers);

// 更新游戏逻辑（等价于 TB_AdvanceSimulation；须先 TB_SetFrameInputs）
TANKBATTLE_API void TB_Update(TankBattleGame game, float deltaTime);

TANKBATTLE_API unsigned int TB_GetFrame(TankBattleGame game);

TANKBATTLE_API unsigned char TB_SetFrameInputs(
    TankBattleGame game,
    unsigned int frame,
    const TB_PlayerInput* inputs,
    unsigned int count);

TANKBATTLE_API void TB_AdvanceSimulation(TankBattleGame game);

// 返回权威逻辑帧固定步长（秒），与 kFixedLogicDeltaTime 一致
TANKBATTLE_API float TB_GetFixedLogicDeltaTime();

// 添加玩家
TANKBATTLE_API unsigned int TB_AddPlayer(TankBattleGame game, const char* name, int faction);

// 移除玩家
TANKBATTLE_API void TB_RemovePlayer(TankBattleGame game, unsigned int playerId);

// 每阵营槽位数（可接管席位）
TANKBATTLE_API void TB_SetSlotsPerFaction(TankBattleGame game, unsigned int slots);
TANKBATTLE_API unsigned int TB_GetSlotsPerFaction(TankBattleGame game);
TANKBATTLE_API unsigned int TB_CountFreeAISlots(TankBattleGame game, int faction);

// 接管阵营 AI 坦克；成功返回 playerId，outTankId 可为 NULL
TANKBATTLE_API unsigned int TB_PossessAITank(
    TankBattleGame game,
    const char* name,
    int faction,
    unsigned int* outTankId);

// 交回 AI 控制权
TANKBATTLE_API unsigned char TB_ReleaseToAI(TankBattleGame game, unsigned int playerId);

// 按指定 id 接管 / 交回（FrameSync 控制事件，幂等）
TANKBATTLE_API unsigned char TB_ApplyPossess(
    TankBattleGame game,
    unsigned int playerId,
    const char* name,
    int faction,
    unsigned int tankId);
TANKBATTLE_API unsigned char TB_ApplyRelease(TankBattleGame game, unsigned int playerId);

TANKBATTLE_API void TB_ClearAiMemory(TankBattleGame game);

// 同步对比：将 FormatCompareSnapshot 写入 buf（UTF-8），返回写入字节数（不含 \\0）；
// 若 buf 不够，仍返回完整长度，内容截断并保证 \\0 结尾。
TANKBATTLE_API int TB_FormatCompareSnapshot(
    TankBattleGame game,
    const char* side,
    char* buf,
    int bufSize);

#pragma pack(push, 1)
typedef struct
{
    unsigned int frame;
    int gameState;
    unsigned int randomSeed;
    unsigned int slotsPerFaction;
    unsigned int nextPlayerId;
    unsigned int nextTankId;
    unsigned int nextBulletId;
    unsigned int factionKills[4];
    unsigned int factionDeaths[4];
} TB_LogicSnapshotHeader;

typedef struct
{
    unsigned int id;
    unsigned int playerId;
    int faction;
    int type;
    int posX;
    int posY;
    int velX;
    int velY;
    int rotation;
    int turretRotation;
    int hp;
    int maxHp;
    int shieldFrames;
    int speedBoostFrames;
    int rapidFireFrames;
    int abilityCooldownFrames;
    int reloadFrames;
    int reloadDurationFrames;
    int recoilVelX;
    int recoilVelY;
    unsigned int lockedTargetId;
    unsigned int aiMoveMode;
    int respawnFrames;
    int spawnProtectionFrames;
    unsigned char chargedShot;
    unsigned char isPlayer;
    unsigned char isAlive;
    unsigned char _pad; // 凑齐 100 字节，避免 C# Marshal.SizeOf 与原生 stride 不一致
} TB_TankLogicSnapshot;

typedef struct
{
    unsigned int id;
    unsigned int ownerId;
    int posX;
    int posY;
    int velX;
    int velY;
    int damage;
    int lifeFrames;
    unsigned char penetrating;
    unsigned char _pad0;
    unsigned char _pad1;
    unsigned char _pad2;
    unsigned int damagedCount;
    unsigned int damagedTankIds[8];
} TB_BulletLogicSnapshot;

typedef struct
{
    unsigned int id;
    const char* name;
    int faction;
    unsigned int kills;
    unsigned int score;
    unsigned char isConnected;
} TB_PlayerLogicSnapshot;
#pragma pack(pop)

// 逐条写入逻辑快照（避免托管数组 stride 与原生结构不对齐）
#pragma pack(push, 1)
typedef struct
{
    unsigned int tankId;
    int wanderHeading;
    int strafeSign;
    int strafeSwitchFrames;
    unsigned int wanderGoalSerial;
    int pathGoalX;
    int pathGoalY;
    unsigned int pathTargetId;
    unsigned int pathMoveMode;
    int pathRecalcFrames;
    int wanderPathGoalX;
    int wanderPathGoalY;
    int wanderPathFrames;
    unsigned int pathWaypointIndex;
    unsigned int waypointCoordCount; // pathWaypointCoords 有效 int 个数（偶数）
    int pathWaypointCoords[64];      // 最多 32 个航点
} TB_AiMemorySnapshot;
#pragma pack(pop)

TANKBATTLE_API void TB_LogicSnap_Begin(TankBattleGame game, const TB_LogicSnapshotHeader* header);
TANKBATTLE_API void TB_LogicSnap_AddTank(TankBattleGame game, const TB_TankLogicSnapshot* tank);
TANKBATTLE_API void TB_LogicSnap_AddBullet(TankBattleGame game, const TB_BulletLogicSnapshot* bullet);
TANKBATTLE_API void TB_LogicSnap_AddAiMemory(TankBattleGame game, const TB_AiMemorySnapshot* memory);
TANKBATTLE_API void TB_LogicSnap_AddPlayer(
    TankBattleGame game,
    unsigned int id,
    const char* name,
    int faction,
    unsigned int kills,
    unsigned int score,
    unsigned char isConnected);
TANKBATTLE_API unsigned char TB_LogicSnap_Commit(TankBattleGame game);
TANKBATTLE_API unsigned int TB_LogicSnap_AiMemoryStructSize();

// 调试：原生结构体字节数（应与 C# Marshal.SizeOf 一致）
TANKBATTLE_API unsigned int TB_LogicSnap_TankStructSize();
TANKBATTLE_API unsigned int TB_LogicSnap_BulletStructSize();
TANKBATTLE_API unsigned int TB_LogicSnap_HeaderStructSize();

// 处理玩家输入
TANKBATTLE_API void TB_ProcessPlayerInput(TankBattleGame game, const TB_PlayerInput* input);

// 开始游戏
TANKBATTLE_API void TB_StartGame(TankBattleGame game);

// 仅生成各阵营 AI 槽位后开局（接管模式）
TANKBATTLE_API void TB_StartGameAIOnly(TankBattleGame game);

// 重置游戏
TANKBATTLE_API void TB_Reset(TankBattleGame game);

// 获取游戏状态
TANKBATTLE_API TB_GameSnapshot* TB_GetGameState(TankBattleGame game);

// 释放游戏状态
TANKBATTLE_API void TB_FreeGameState(TB_GameSnapshot* snapshot);

// 获取玩家信息
TANKBATTLE_API TB_PlayerInfo* TB_GetPlayersInfo(TankBattleGame game, unsigned int* count);

// 释放玩家信息
TANKBATTLE_API void TB_FreePlayersInfo(TB_PlayerInfo* players, unsigned int count);

// 获取阵营状态
TANKBATTLE_API TB_FactionStatus* TB_GetFactionStatus(TankBattleGame game, unsigned int* count);

// 释放阵营状态
TANKBATTLE_API void TB_FreeFactionStatus(TB_FactionStatus* status, unsigned int count);

// 检查游戏是否结束
TANKBATTLE_API unsigned char TB_IsGameOver(TankBattleGame game);

// 获取获胜阵营
TANKBATTLE_API int TB_GetWinner(TankBattleGame game);

// 设置随机种子
TANKBATTLE_API void TB_SetRandomSeed(TankBattleGame game, unsigned int seed);

// 从 JSON 文本加载地图阻挡墙
TANKBATTLE_API unsigned char TB_LoadMapObstaclesJson(TankBattleGame game, const char* json);

#ifdef __cplusplus
}
#endif
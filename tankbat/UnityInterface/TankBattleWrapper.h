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

// 更新游戏逻辑
    // deltaTime 保留兼容 Unity 调用，Core 固定 30Hz 每帧一步，参数忽略
    TANKBATTLE_API void TB_Update(TankBattleGame game, float deltaTime);

    // 返回权威逻辑帧固定步长（秒），与 kFixedLogicDeltaTime 一致
    TANKBATTLE_API float TB_GetFixedLogicDeltaTime();

// 添加玩家
TANKBATTLE_API unsigned int TB_AddPlayer(TankBattleGame game, const char* name, int faction);

// 移除玩家
TANKBATTLE_API void TB_RemovePlayer(TankBattleGame game, unsigned int playerId);

// 处理玩家输入
TANKBATTLE_API void TB_ProcessPlayerInput(TankBattleGame game, const TB_PlayerInput* input);

// 开始游戏
TANKBATTLE_API void TB_StartGame(TankBattleGame game);

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
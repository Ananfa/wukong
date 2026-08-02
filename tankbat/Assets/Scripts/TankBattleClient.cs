using System;
using System.Collections.Generic;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using UnityEngine;
using TankBattle.Network;

namespace TankBattle
{
    /// <summary>与 C++ kLogicTickRate / kFixedLogicDeltaTime 一致。</summary>
    public static class LogicTiming
    {
        public const float TickRate = 30f;
        public const float FixedDeltaTime = 1f / TickRate;
        public const int MaxCatchUpStepsPerFrame = 5;
    }

    // C#与C++的接口定义
    public static class TankBattleNative
    {
        // 数据结构
        [StructLayout(LayoutKind.Sequential)]
        public struct TB_Vector2
        {
            public float x;
            public float y;
            
            public TB_Vector2(float x, float y)
            {
                this.x = x;
                this.y = y;
            }
            
            public static implicit operator Vector2(TB_Vector2 v) => new Vector2(v.x, v.y);
            public static implicit operator TB_Vector2(Vector2 v) => new TB_Vector2(v.x, v.y);
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct TB_TankState
        {
            public uint id;
            public uint playerId;
            public int faction;
            public int type;
            public TB_Vector2 position;
            public TB_Vector2 velocity;
            public float rotation;
            public float turretRotation;
            public float hp;
            public float maxHp;
            public float shield;
            public float speedBoost;
            public float rapidFire;
            public float abilityCooldown;
            [MarshalAs(UnmanagedType.U1)]
            public bool isAlive;
            public uint lockedTargetId;
            public uint aiMoveMode;
            public float moveSpeed;
            public float respawnTimeRemaining;
            public float reloadTimeRemaining;
            public float reloadDuration;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct TB_BulletState
        {
            public uint id;
            public uint ownerId;
            public TB_Vector2 position;
            public TB_Vector2 velocity;
            public float damage;
            public float lifeTime;
            [MarshalAs(UnmanagedType.U1)]
            public bool penetrating;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct TB_ObstacleWallState
        {
            public float centerX;
            public float centerY;
            public float width;
            public float height;
            public float rotation;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct TB_GameSnapshot
        {
            public uint frame;
            public ulong timestamp;
            public IntPtr tanks;
            public uint tankCount;
            public IntPtr bullets;
            public uint bulletCount;
            public IntPtr obstacles;
            public uint obstacleCount;
            public int state;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct TB_PlayerInfo
        {
            public uint id;
            public IntPtr name;
            public int faction;
            public uint kills;
            public uint score;
            [MarshalAs(UnmanagedType.U1)]
            public bool isConnected;
        }
        
        [StructLayout(LayoutKind.Sequential)]
        public struct TB_FactionStatus
        {
            public int faction;
            public uint aliveCount;
            public uint totalCount;
            public uint kills;
            public uint deaths;
        }
        
        [StructLayout(LayoutKind.Sequential, Pack = 1)]
        public struct TB_PlayerInput
        {
            public uint playerId;
            public uint frame;
            public short moveX;
            public short moveY;
            public short aimX;
            public short aimY;
            [MarshalAs(UnmanagedType.U1)]
            public bool fire;
            [MarshalAs(UnmanagedType.U1)]
            public bool useAbility;
            public ulong timestamp;
        }
        
        // DLL导入
        [DllImport("TankBattleNative")]
        private static extern IntPtr TB_CreateGame();
        
        [DllImport("TankBattleNative")]
        private static extern void TB_DestroyGame(IntPtr game);
        
        [DllImport("TankBattleNative")]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool TB_Initialize(IntPtr game, uint maxPlayers);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_Update(IntPtr game, float deltaTime);

        [DllImport("TankBattleNative")]
        private static extern uint TB_GetFrame(IntPtr game);

        [DllImport("TankBattleNative")]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool TB_SetFrameInputs(IntPtr game, uint frame, IntPtr inputs, uint count);

        [DllImport("TankBattleNative")]
        private static extern void TB_AdvanceSimulation(IntPtr game);
        
        [DllImport("TankBattleNative")]
        private static extern uint TB_AddPlayer(IntPtr game, string name, int faction);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_RemovePlayer(IntPtr game, uint playerId);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_ProcessPlayerInput(IntPtr game, ref TB_PlayerInput input);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_StartGame(IntPtr game);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_Reset(IntPtr game);
        
        [DllImport("TankBattleNative")]
        private static extern IntPtr TB_GetGameState(IntPtr game);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_FreeGameState(IntPtr snapshot);
        
        [DllImport("TankBattleNative")]
        private static extern IntPtr TB_GetPlayersInfo(IntPtr game, out uint count);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_FreePlayersInfo(IntPtr players, uint count);
        
        [DllImport("TankBattleNative")]
        private static extern IntPtr TB_GetFactionStatus(IntPtr game, out uint count);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_FreeFactionStatus(IntPtr status, uint count);
        
        [DllImport("TankBattleNative")]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool TB_IsGameOver(IntPtr game);
        
        [DllImport("TankBattleNative")]
        private static extern int TB_GetWinner(IntPtr game);
        
        [DllImport("TankBattleNative")]
        private static extern void TB_SetRandomSeed(IntPtr game, uint seed);

        [DllImport("TankBattleNative")]
        [return: MarshalAs(UnmanagedType.U1)]
        private static extern bool TB_LoadMapObstaclesJson(IntPtr game, string json);
        
        // 包装类
        public class GameCore : IDisposable
        {
            private IntPtr nativeHandle;
            private bool disposed = false;
            
            public GameCore()
            {
                nativeHandle = TB_CreateGame();
            }
            
            public bool Initialize(uint maxPlayers = 8)
            {
                return TB_Initialize(nativeHandle, maxPlayers);
            }
            
            public void Update()
            {
                TB_AdvanceSimulation(nativeHandle);
            }

            public uint GetFrame()
            {
                return TB_GetFrame(nativeHandle);
            }

            public bool SetFrameInputs(uint frame, PlayerInput[] inputs)
            {
                if (inputs == null || inputs.Length == 0)
                    return TB_SetFrameInputs(nativeHandle, frame, IntPtr.Zero, 0);

                int elemSize = Marshal.SizeOf<TB_PlayerInput>();
                IntPtr buffer = Marshal.AllocHGlobal(elemSize * inputs.Length);
                try
                {
                    for (int i = 0; i < inputs.Length; i++)
                    {
                        TB_PlayerInput nativeInput = ToNativeInput(inputs[i]);
                        Marshal.StructureToPtr(nativeInput, buffer + i * elemSize, false);
                    }

                    return TB_SetFrameInputs(nativeHandle, frame, buffer, (uint)inputs.Length);
                }
                finally
                {
                    Marshal.FreeHGlobal(buffer);
                }
            }

            public void AdvanceSimulation()
            {
                TB_AdvanceSimulation(nativeHandle);
            }
            
            public uint AddPlayer(string name, Faction faction)
            {
                return TB_AddPlayer(nativeHandle, name, (int)faction);
            }
            
            public void RemovePlayer(uint playerId)
            {
                TB_RemovePlayer(nativeHandle, playerId);
            }
            
            public void ProcessPlayerInput(PlayerInput input)
            {
                TB_PlayerInput nativeInput = ToNativeInput(input);
                TB_ProcessPlayerInput(nativeHandle, ref nativeInput);
            }

            private static TB_PlayerInput ToNativeInput(PlayerInput input)
            {
                return new TB_PlayerInput
                {
                    playerId = input.playerId,
                    frame = input.frame,
                    moveX = input.moveX,
                    moveY = input.moveY,
                    aimX = input.aimX,
                    aimY = input.aimY,
                    fire = input.fire,
                    useAbility = input.useAbility,
                    timestamp = input.timestamp
                };
            }
            
            public void StartGame()
            {
                TB_StartGame(nativeHandle);
            }
            
            public void Reset()
            {
                TB_Reset(nativeHandle);
            }
            
            public GameSnapshot GetGameState()
            {
                IntPtr snapshotPtr = TB_GetGameState(nativeHandle);
                if (snapshotPtr == IntPtr.Zero)
                    return null;
                
                TB_GameSnapshot nativeSnapshot = Marshal.PtrToStructure<TB_GameSnapshot>(snapshotPtr);
                
                var snapshot = new GameSnapshot
                {
                    frame = nativeSnapshot.frame,
                    timestamp = nativeSnapshot.timestamp,
                    state = (GameState)nativeSnapshot.state
                };
                
                // 复制坦克数据
                snapshot.tanks = new TankState[nativeSnapshot.tankCount];
                int tankSize = Marshal.SizeOf<TB_TankState>();
                for (int i = 0; i < nativeSnapshot.tankCount; i++)
                {
                    IntPtr tankPtr = new IntPtr(nativeSnapshot.tanks.ToInt64() + i * tankSize);
                    TB_TankState nativeTank = Marshal.PtrToStructure<TB_TankState>(tankPtr);
                    
                    snapshot.tanks[i] = new TankState
                    {
                        id = nativeTank.id,
                        playerId = nativeTank.playerId,
                        faction = (Faction)nativeTank.faction,
                        type = (TankType)nativeTank.type,
                        position = nativeTank.position,
                        velocity = nativeTank.velocity,
                        rotation = nativeTank.rotation,
                        turretRotation = nativeTank.turretRotation,
                        hp = nativeTank.hp,
                        maxHp = nativeTank.maxHp,
                        shield = nativeTank.shield,
                        speedBoost = nativeTank.speedBoost,
                        rapidFire = nativeTank.rapidFire,
                        abilityCooldown = nativeTank.abilityCooldown,
                        isAlive = nativeTank.isAlive,
                        lockedTargetId = nativeTank.lockedTargetId,
                        aiMoveMode = nativeTank.aiMoveMode,
                        moveSpeed = nativeTank.moveSpeed,
                        respawnTimeRemaining = nativeTank.respawnTimeRemaining,
                        reloadTimeRemaining = nativeTank.reloadTimeRemaining,
                        reloadDuration = nativeTank.reloadDuration
                    };
                }
                
                // 复制子弹数据
                snapshot.bullets = new BulletState[nativeSnapshot.bulletCount];
                int bulletSize = Marshal.SizeOf<TB_BulletState>();
                for (int i = 0; i < nativeSnapshot.bulletCount; i++)
                {
                    IntPtr bulletPtr = new IntPtr(nativeSnapshot.bullets.ToInt64() + i * bulletSize);
                    TB_BulletState nativeBullet = Marshal.PtrToStructure<TB_BulletState>(bulletPtr);
                    
                    snapshot.bullets[i] = new BulletState
                    {
                        id = nativeBullet.id,
                        ownerId = nativeBullet.ownerId,
                        position = nativeBullet.position,
                        velocity = nativeBullet.velocity,
                        damage = nativeBullet.damage,
                        lifeTime = nativeBullet.lifeTime,
                        penetrating = nativeBullet.penetrating
                    };
                }
                
                snapshot.obstacles = new ObstacleWallState[nativeSnapshot.obstacleCount];
                int obstacleSize = Marshal.SizeOf<TB_ObstacleWallState>();
                for (int i = 0; i < nativeSnapshot.obstacleCount; i++)
                {
                    IntPtr obstaclePtr = new IntPtr(nativeSnapshot.obstacles.ToInt64() + i * obstacleSize);
                    TB_ObstacleWallState nativeObstacle = Marshal.PtrToStructure<TB_ObstacleWallState>(obstaclePtr);
                    snapshot.obstacles[i] = new ObstacleWallState
                    {
                        centerX = nativeObstacle.centerX,
                        centerY = nativeObstacle.centerY,
                        width = nativeObstacle.width,
                        height = nativeObstacle.height,
                        rotation = nativeObstacle.rotation
                    };
                }
                
                TB_FreeGameState(snapshotPtr);
                return snapshot;
            }
            
            public List<PlayerInfo> GetPlayersInfo()
            {
                uint count = 0;
                IntPtr playersPtr = TB_GetPlayersInfo(nativeHandle, out count);
                if (playersPtr == IntPtr.Zero)
                    return new List<PlayerInfo>();
                
                var players = new List<PlayerInfo>((int)count);
                int playerSize = Marshal.SizeOf<TB_PlayerInfo>();
                
                for (int i = 0; i < count; i++)
                {
                    IntPtr playerPtr = new IntPtr(playersPtr.ToInt64() + i * playerSize);
                    TB_PlayerInfo nativePlayer = Marshal.PtrToStructure<TB_PlayerInfo>(playerPtr);
                    
                    players.Add(new PlayerInfo
                    {
                        id = nativePlayer.id,
                        name = Marshal.PtrToStringAnsi(nativePlayer.name),
                        faction = (Faction)nativePlayer.faction,
                        kills = nativePlayer.kills,
                        score = nativePlayer.score,
                        isConnected = nativePlayer.isConnected
                    });
                }
                
                TB_FreePlayersInfo(playersPtr, count);
                return players;
            }
            
            public List<FactionStatus> GetFactionStatus()
            {
                uint count = 0;
                IntPtr statusPtr = TB_GetFactionStatus(nativeHandle, out count);
                if (statusPtr == IntPtr.Zero)
                    return new List<FactionStatus>();
                
                var statusList = new List<FactionStatus>((int)count);
                int statusSize = Marshal.SizeOf<TB_FactionStatus>();
                
                for (int i = 0; i < count; i++)
                {
                    IntPtr statusItemPtr = new IntPtr(statusPtr.ToInt64() + i * statusSize);
                    TB_FactionStatus nativeStatus = Marshal.PtrToStructure<TB_FactionStatus>(statusItemPtr);
                    
                    statusList.Add(new FactionStatus
                    {
                        faction = (Faction)nativeStatus.faction,
                        aliveCount = nativeStatus.aliveCount,
                        totalCount = nativeStatus.totalCount,
                        kills = nativeStatus.kills,
                        deaths = nativeStatus.deaths
                    });
                }
                
                TB_FreeFactionStatus(statusPtr, count);
                return statusList;
            }
            
            public bool IsGameOver()
            {
                return TB_IsGameOver(nativeHandle);
            }
            
            public Faction GetWinner()
            {
                return (Faction)TB_GetWinner(nativeHandle);
            }
            
            public void SetRandomSeed(uint seed)
            {
                TB_SetRandomSeed(nativeHandle, seed);
            }

            public bool LoadMapObstacles(string json)
            {
                if (string.IsNullOrEmpty(json))
                    return false;
                return TB_LoadMapObstaclesJson(nativeHandle, json);
            }
            
            public void Dispose()
            {
                Dispose(true);
                GC.SuppressFinalize(this);
            }
            
            private void Dispose(bool disposing)
            {
                if (!disposed)
                {
                    if (nativeHandle != IntPtr.Zero)
                    {
                        TB_DestroyGame(nativeHandle);
                        nativeHandle = IntPtr.Zero;
                    }
                    disposed = true;
                }
            }
            
            ~GameCore()
            {
                Dispose(false);
            }
        }
    }
    
    // C#端数据结构
    public enum Faction
    {
        Soviet = 0,
        USA = 1,
        Germany = 2,
        Italy = 3
    }
    
    public enum TankType
    {
        T34 = 0,
        KV1 = 1,
        KV2 = 2,
        M3 = 3,
        M4 = 4,
        Panther = 5,
        Tiger = 6,
        L6_40 = 7,
        P40 = 8
    }
    
    public enum GameState
    {
        Waiting = 0,
        Playing = 1,
        Ended = 2
    }
    
    public struct Vector2
    {
        public float x;
        public float y;
        
        public Vector2(float x, float y)
        {
            this.x = x;
            this.y = y;
        }
        
        public float Length => Mathf.Sqrt(x * x + y * y);
        
        public Vector2 Normalized
        {
            get
            {
                float len = Length;
                if (len > 0.0001f)
                    return new Vector2(x / len, y / len);
                return new Vector2(0, 0);
            }
        }
        
        public static Vector2 operator +(Vector2 a, Vector2 b) => new Vector2(a.x + b.x, a.y + b.y);
        public static Vector2 operator -(Vector2 a, Vector2 b) => new Vector2(a.x - b.x, a.y - b.y);
        public static Vector2 operator *(Vector2 v, float s) => new Vector2(v.x * s, v.y * s);
        public static Vector2 operator /(Vector2 v, float s) => new Vector2(v.x / s, v.y / s);
    }
    
    public struct PlayerInput
    {
        public const int DirectionScale = 32767;

        public uint playerId;
        public uint frame;
        public short moveX;
        public short moveY;
        public short aimX;
        public short aimY;
        public bool fire;
        public bool useAbility;
        public ulong timestamp;

        public static short QuantizeDirection(float component)
        {
            return (short)Mathf.Clamp(Mathf.RoundToInt(component * DirectionScale), -DirectionScale, DirectionScale);
        }

        public static Vector2 DequantizeDirection(short x, short y)
        {
            return new Vector2(x / (float)DirectionScale, y / (float)DirectionScale);
        }
    }
    
    public enum AiMoveMode : uint
    {
        None = 0,
        WanderNoTarget = 1,
        ApproachTarget = 2,
        RetreatFromTarget = 3,
        StrafeTarget = 4,
        AvoidObstacle = 5,
        FollowPath = 6
    }

    public struct TankState
    {
        public uint id;
        public uint playerId;
        public Faction faction;
        public TankType type;
        public Vector2 position;
        public Vector2 velocity;
        public float rotation;
        public float turretRotation;
        public float hp;
        public float maxHp;
        public float shield;
        public float speedBoost;
        public float rapidFire;
        public float abilityCooldown;
        public bool isAlive;
        public uint lockedTargetId;
        public uint aiMoveMode;
        public float moveSpeed;
        public float respawnTimeRemaining;
        public float reloadTimeRemaining;
        public float reloadDuration;
    }
    
    public struct BulletState
    {
        public uint id;
        public uint ownerId;
        public Vector2 position;
        public Vector2 velocity;
        public float damage;
        public float lifeTime;
        public bool penetrating;
    }
    
    public struct ObstacleWallState
    {
        public float centerX;
        public float centerY;
        public float width;
        public float height;
        public float rotation;
    }

    public class GameSnapshot
    {
        public uint frame;
        public ulong timestamp;
        public TankState[] tanks;
        public BulletState[] bullets;
        public ObstacleWallState[] obstacles;
        public GameState state;
    }
    
    public class PlayerInfo
    {
        public uint id;
        public string name;
        public Faction faction;
        public uint kills;
        public uint score;
        public bool isConnected;
    }
    
    public class FactionStatus
    {
        public Faction faction;
        public uint aliveCount;
        public uint totalCount;
        public uint kills;
        public uint deaths;
    }
    
    // Unity 客户端：本地 LocalFrameDriver；在线 = HTTP登录→TCP进Lobby→StartBattle→KCP帧同步
    public class TankBattleClient : MonoBehaviour
    {
        private TankBattleNative.GameCore gameCore;
        private uint playerId;
        private string playerName = "Player";
        private Faction selectedFaction = Faction.Soviet;

        [Tooltip("启用后走完整 wukong 在线流程：Login→创角/选角→进游戏→开战斗房间→KCP")]
        [SerializeField] private bool useNetworkTransport = true;

        [Header("Online Login (HTTP)")]
        [SerializeField] private string loginBaseUrl = "http://127.0.0.1:11000";
        [SerializeField] private string openId = "tankbat_player";
        [SerializeField] private uint gameServerId = 1;
        [SerializeField] private uint battleDefId = 1;

        [Header("Online Battle fallback (skip Lobby, direct KCP — debug only)")]
        [SerializeField] private bool useDirectKcpBypass = false;
        [SerializeField] private string battleKcpHost = "127.0.0.1";
        [SerializeField] private int battleKcpPort = 19001;
        [SerializeField] private ulong battleRoomId = 1;
        [SerializeField] private ulong battleRoleId = 1;
        [SerializeField] private string battleSessionToken = "";

        private bool allowLocalInput;
        private bool gameOverNotified;
        private Vector2 cachedPlayerTankPlanarPos;
        private Vector2 cachedAimDirection = new Vector2(1f, 0f);

        private LocalFrameDriver localFrameDriver;
        private KcpBattleClient kcpBattleClient;
        private NetworkFrameDriver networkFrameDriver;
        private GatewaySession gatewaySession;
        private ulong onlineRoleId;
        private bool onlineLobbyReady;

        // 事件委托
        public delegate void GameStateChangedHandler(GameSnapshot snapshot);
        public delegate void PlayerJoinedHandler(PlayerInfo player);
        public delegate void PlayerLeftHandler(uint playerId);
        public delegate void GameStartedHandler();
        public delegate void GameEndedHandler(Faction winner);
        
        public event GameStateChangedHandler OnGameStateChanged;
        public event PlayerJoinedHandler OnPlayerJoined;
        public event PlayerLeftHandler OnPlayerLeft;
        public event GameStartedHandler OnGameStarted;
        public event GameEndedHandler OnGameEnded;

        public bool UseNetworkTransport => useNetworkTransport;
        public bool UseDirectKcpBypass => useDirectKcpBypass;

        /// <summary>由 GameManager 在启动时设置；场景里旧序列化值会被覆盖。</summary>
        public void SetUseNetworkTransport(bool enabled)
        {
            useNetworkTransport = enabled;
        }
        /// <summary>已完成 HTTP 登录并进入 Lobby（Gateway 仍保持连接）。</summary>
        public bool IsOnlineLoggedIn =>
            useNetworkTransport && onlineLobbyReady &&
            (useDirectKcpBypass || (gatewaySession != null && gatewaySession.IsRunning));
        public bool IsInBattle =>
            useNetworkTransport && kcpBattleClient != null && kcpBattleClient.IsRunning;
        public string LoginOpenId => openId;
        
        private void Awake()
        {
            gameCore = new TankBattleNative.GameCore();
            gameCore.Initialize(8);
            localFrameDriver = new LocalFrameDriver(gameCore);
            kcpBattleClient = new KcpBattleClient();
            networkFrameDriver = new NetworkFrameDriver(gameCore, kcpBattleClient, BuildLocalPlayerInput);
            networkFrameDriver.OnSimulationStarted += OnNetworkSimulationStarted;
            networkFrameDriver.OnSimulationEnded += OnNetworkSimulationEnded;
            gatewaySession = new GatewaySession();
            gatewaySession.OnDisconnected += () =>
            {
                onlineLobbyReady = false;
            };
        }

        /// <summary>供 GameManager 调用，与 Awake 中的初始化兼容。</summary>
        public void Initialize()
        {
        }

        /// <summary>本地模式：在 Native 中注册玩家并返回 playerId。</summary>
        public uint ConnectToServer(string name, Faction faction)
        {
            if (useNetworkTransport)
            {
                playerName = string.IsNullOrEmpty(name) ? "Player" : name;
                selectedFaction = faction;
                return 0; // online: playerId assigned after Snapshot
            }

            if (gameCore == null) return 0;
            playerName = string.IsNullOrEmpty(name) ? "Player" : name;
            selectedFaction = faction;
            uint id = gameCore.AddPlayer(playerName, faction);
            if (id == 0) return 0;
            playerId = id;
            OnPlayerJoined?.Invoke(new PlayerInfo
            {
                id = id,
                name = playerName,
                faction = faction,
                kills = 0,
                score = 0,
                isConnected = true
            });
            return id;
        }

        public void RequestStartGame()
        {
            if (useNetworkTransport)
            {
                if (useDirectKcpBypass)
                {
                    _ = EnterBattleAsync(BuildInspectorSession());
                    return;
                }

                if (!IsOnlineLoggedIn)
                {
                    Debug.LogError("TankBattleClient: 尚未登录 Lobby，请先登录再开始游戏");
                    return;
                }

                _ = StartOnlineBattleAsync();
                return;
            }

            if (gameCore == null) return;
            gameOverNotified = false;
            uint seed = (uint)(DateTime.UtcNow.Ticks & 0xFFFFFFFF);
            gameCore.SetRandomSeed(seed);
            gameCore.StartGame();
            allowLocalInput = true;
            localFrameDriver.Reset();
            OnGameStarted?.Invoke();
        }

        /// <summary>
        /// HTTP login/createRole/enterGame → Gateway Auth → ENTERLOBBY。
        /// 只做一次；之后开始游戏只开战斗房间。
        /// </summary>
        public async Task<bool> LoginAndEnterLobbyAsync(string accountOpenId = null)
        {
            if (!useNetworkTransport)
            {
                Debug.LogWarning("TankBattleClient: 本地模式无需登录");
                return true;
            }

            if (useDirectKcpBypass)
            {
                onlineLobbyReady = true;
                Debug.LogWarning("TankBattleClient: direct KCP bypass，跳过 Login/Lobby");
                return true;
            }

            if (IsOnlineLoggedIn)
                return true;

            try
            {
                onlineLobbyReady = false;
                if (!string.IsNullOrEmpty(accountOpenId))
                    openId = accountOpenId.Trim();

                string accountName = string.IsNullOrEmpty(openId) ? playerName : openId;
                if (string.IsNullOrEmpty(playerName) || playerName == "Player")
                    playerName = accountName;

                Debug.Log($"TankBattleClient: login openId={accountName}");

                var account = await LoginHttpClient.LoginAsync(loginBaseUrl, accountName);
                if (account.RoleId == 0)
                {
                    Debug.Log("TankBattleClient: no role, createRole...");
                    account.RoleId = await LoginHttpClient.CreateRoleAsync(
                        loginBaseUrl, account.UserId, account.Token, gameServerId, accountName);
                }

                onlineRoleId = account.RoleId;
                Debug.Log($"TankBattleClient: enterGame userId={account.UserId} roleId={account.RoleId}");

                var enter = await LoginHttpClient.EnterGameAsync(
                    loginBaseUrl, account.UserId, account.RoleId, account.Token, gameServerId);

                string cipher = Guid.NewGuid().ToString("N");
                bool authOk = await gatewaySession.ConnectAndAuthAsync(
                    enter.Host, enter.Port, account.UserId, enter.GToken, enter.GateId, cipher);
                if (!authOk)
                {
                    Debug.LogError("TankBattleClient: gateway auth connect failed");
                    return false;
                }

                if (!await WaitTaskWithTimeout(gatewaySession.WaitEnterLobbyAsync(), 15f, "ENTERLOBBY"))
                {
                    gatewaySession.Disconnect();
                    return false;
                }

                onlineLobbyReady = true;
                Debug.Log("TankBattleClient: entered Lobby (ready to StartBattle)");
                return true;
            }
            catch (Exception ex)
            {
                onlineLobbyReady = false;
                Debug.LogError("TankBattleClient login failed: " + ex);
                return false;
            }
        }

        /// <summary>已在 Lobby：StartBattle → 拿房间地址 → KCP 开战。</summary>
        public async Task<bool> StartOnlineBattleAsync()
        {
            try
            {
                gameOverNotified = false;
                allowLocalInput = false;
                networkFrameDriver.Reset();

                if (useDirectKcpBypass)
                    return await EnterBattleAsync(BuildInspectorSession());

                if (!IsOnlineLoggedIn)
                {
                    Debug.LogError("TankBattleClient: StartOnlineBattle requires Lobby session");
                    return false;
                }

                // 若上一局 KCP 未清干净，先退房间
                if (kcpBattleClient != null && kcpBattleClient.IsRunning)
                    LeaveBattle();

                Debug.Log($"TankBattleClient: request StartBattle battleDefId={battleDefId}");
                var battleTask = gatewaySession.RequestStartBattleAsync(battleDefId, (int)selectedFaction);
                if (!await WaitTaskWithTimeout(battleTask, 15f, "BATTLE_ENTER"))
                    return false;

                if (battleTask.IsFaulted)
                {
                    Debug.LogError("TankBattleClient: StartBattle failed: " +
                                   battleTask.Exception?.GetBaseException().Message);
                    return false;
                }

                var info = battleTask.Result;
                var session = new BattleEnterSession
                {
                    KcpHost = info.KcpHost,
                    KcpPort = info.KcpPort,
                    RoomId = info.RoomId,
                    RoleId = onlineRoleId,
                    BattleServerId = info.BattleServerId,
                    SessionToken = info.SessionToken
                };
                return await EnterBattleAsync(session);
            }
            catch (Exception ex)
            {
                Debug.LogError("TankBattleClient StartOnlineBattle failed: " + ex);
                return false;
            }
        }

        /// <summary>兼容旧调用：未登录则先登录再开战（一般由 UI 分步调用）。</summary>
        public async Task<bool> StartOnlineFlowAsync()
        {
            if (!IsOnlineLoggedIn && !await LoginAndEnterLobbyAsync())
                return false;
            return await StartOnlineBattleAsync();
        }

        /// <summary>Online: connect Battle KCP with Lobby-issued enter info (or inspector bypass).</summary>
        public async Task<bool> EnterBattleAsync(BattleEnterSession session)
        {
            if (gameCore == null || session == null)
                return false;

            useNetworkTransport = true;
            gameOverNotified = false;
            allowLocalInput = false;
            networkFrameDriver.Reset();

            bool ok = await kcpBattleClient.ConnectAndAuthAsync(session);
            if (!ok)
            {
                Debug.LogError("TankBattleClient: Battle KCP connect failed");
                return false;
            }
            return true;
        }

        /// <summary>退出战斗房间（KCP），保留 Gateway/Lobby 登录态。</summary>
        public void LeaveBattle()
        {
            allowLocalInput = false;
            if (kcpBattleClient != null)
            {
                try
                {
                    if (kcpBattleClient.IsRunning)
                        kcpBattleClient.LeaveRoom();
                }
                catch (Exception ex)
                {
                    Debug.LogWarning("LeaveBattle LeaveRoom: " + ex.Message);
                }
                kcpBattleClient.Disconnect();
            }

            networkFrameDriver?.Reset();
            gameCore?.Reset();
            playerId = 0;
            gameOverNotified = false;
        }

        /// <summary>退出战斗并断开 Gateway（需重新登录）。</summary>
        public void Logout()
        {
            LeaveBattle();
            onlineLobbyReady = false;
            onlineRoleId = 0;
            gatewaySession?.Disconnect();
        }

        private static async Task<bool> WaitTaskWithTimeout(Task task, float timeoutSeconds, string label)
        {
            float start = Time.realtimeSinceStartup;
            while (!task.IsCompleted)
            {
                if (Time.realtimeSinceStartup - start > timeoutSeconds)
                {
                    Debug.LogError($"TankBattleClient: wait {label} timeout");
                    return false;
                }
                await Task.Yield();
            }

            if (task.IsCanceled)
            {
                Debug.LogError($"TankBattleClient: {label} canceled");
                return false;
            }

            return true;
        }

        private BattleEnterSession BuildInspectorSession()
        {
            return new BattleEnterSession
            {
                KcpHost = battleKcpHost,
                KcpPort = battleKcpPort,
                RoomId = battleRoomId,
                RoleId = battleRoleId != 0 ? battleRoleId : onlineRoleId,
                SessionToken = battleSessionToken
            };
        }

        private void OnNetworkSimulationStarted()
        {
            playerId = networkFrameDriver.LocalPlayerId;
            // 以服务器 Snapshot 阵营为准（颜色/出生点/镜头）
            selectedFaction = networkFrameDriver.LocalFaction;
            allowLocalInput = true;
            gameOverNotified = false;
            OnPlayerJoined?.Invoke(new PlayerInfo
            {
                id = playerId,
                name = playerName,
                faction = selectedFaction,
                kills = 0,
                score = 0,
                isConnected = true
            });
            OnGameStarted?.Invoke();
        }

        private void OnNetworkSimulationEnded()
        {
            allowLocalInput = false;
            CheckGameState();
        }

        public uint GetCurrentFrame()
        {
            return gameCore != null ? gameCore.GetFrame() : 0;
        }

        public bool LoadMapObstacles(string json)
        {
            if (gameCore == null || string.IsNullOrEmpty(json))
                return false;
            return gameCore.LoadMapObstacles(json);
        }

        public ObstacleWallState[] GetMapObstacles()
        {
            if (gameCore == null)
                return null;
            GameSnapshot snapshot = gameCore.GetGameState();
            return snapshot?.obstacles;
        }

        /// <summary>
        /// 完全断开：退战斗 + 断 Gateway。返回选阵营菜单请用 <see cref="LeaveBattle"/>。
        /// </summary>
        public void Disconnect(bool notifyPlayerLeft = true)
        {
            uint left = playerId;
            if (useNetworkTransport)
            {
                Logout();
            }
            else
            {
                allowLocalInput = false;
                if (gameCore != null && playerId != 0)
                    gameCore.RemovePlayer(playerId);
                playerId = 0;
                gameOverNotified = false;
            }

            if (notifyPlayerLeft && left != 0)
                OnPlayerLeft?.Invoke(left);
        }

        public List<PlayerInfo> GetPlayersInfo()
        {
            if (gameCore == null) return new List<PlayerInfo>();
            return gameCore.GetPlayersInfo();
        }

        public List<FactionStatus> GetFactionStatus()
        {
            if (gameCore == null) return new List<FactionStatus>();
            return gameCore.GetFactionStatus();
        }

        public PlayerInfo GetPlayerInfo(uint id)
        {
            if (gameCore == null) return null;
            foreach (var p in gameCore.GetPlayersInfo())
            {
                if (p.id == id) return p;
            }
            return null;
        }

        private void Start()
        {
        }
        
        private void OnDestroy()
        {
            if (networkFrameDriver != null)
            {
                networkFrameDriver.OnSimulationStarted -= OnNetworkSimulationStarted;
                networkFrameDriver.OnSimulationEnded -= OnNetworkSimulationEnded;
                networkFrameDriver.Dispose();
                networkFrameDriver = null;
            }
            gatewaySession?.Disconnect();
            kcpBattleClient?.Disconnect();
            if (gameCore != null)
            {
                gameCore.Dispose();
                gameCore = null;
            }
        }
        
        private void Update()
        {
            if (useNetworkTransport)
            {
                gatewaySession?.Update();
                networkFrameDriver?.Tick();
            }
            else
            {
                localFrameDriver.Tick(Time.deltaTime, BuildLocalPlayerInput);
            }

            UpdateGameState();
        }

        private PlayerInput? BuildLocalPlayerInput()
        {
            if (!allowLocalInput || playerId == 0)
                return null;

            Vector2 move = ReadMoveAxes();
            Vector2 aim = ReadAimDirection();

            bool fire = Input.GetKey(KeyCode.LeftControl) || Input.GetKey(KeyCode.F) || Input.GetMouseButton(0);
            bool useAbility = Input.GetKey(KeyCode.Space) || Input.GetKey(KeyCode.LeftShift);

            return new PlayerInput
            {
                playerId = playerId,
                moveX = PlayerInput.QuantizeDirection(move.x),
                moveY = PlayerInput.QuantizeDirection(move.y),
                aimX = PlayerInput.QuantizeDirection(aim.x),
                aimY = PlayerInput.QuantizeDirection(aim.y),
                fire = fire,
                useAbility = useAbility,
                timestamp = (ulong)(Time.time * 1000.0)
            };
        }

        private static Vector2 TransformCameraPlanarInput(float strafe, float forward)
        {
            Vector2 cameraInput = new Vector2(strafe, forward);
            if (cameraInput.Length < 0.01f)
                return new Vector2(0f, 0f);

            cameraInput = cameraInput.Normalized;

            Camera cam = Camera.main;
            if (cam == null)
                return cameraInput;

            Vector3 camForward = cam.transform.forward;
            camForward.y = 0f;
            if (camForward.sqrMagnitude < 0.0001f)
                camForward = Vector3.forward;
            else
                camForward.Normalize();

            Vector3 camRight = cam.transform.right;
            camRight.y = 0f;
            if (camRight.sqrMagnitude < 0.0001f)
                camRight = Vector3.right;
            else
                camRight.Normalize();

            // strafe: A 左 / D 右；forward: W 朝屏幕上方
            Vector3 world = camForward * cameraInput.y + camRight * cameraInput.x;
            if (world.sqrMagnitude < 0.0001f)
                return new Vector2(0f, 0f);

            world.Normalize();
            return new Vector2(world.x, world.z);
        }

        private static Vector2 ReadMoveAxes()
        {
            float strafe = (Input.GetKey(KeyCode.D) || Input.GetKey(KeyCode.RightArrow) ? 1f : 0f)
                         - (Input.GetKey(KeyCode.A) || Input.GetKey(KeyCode.LeftArrow) ? 1f : 0f);
            float forward = (Input.GetKey(KeyCode.W) || Input.GetKey(KeyCode.UpArrow) ? 1f : 0f)
                          - (Input.GetKey(KeyCode.S) || Input.GetKey(KeyCode.DownArrow) ? 1f : 0f);
            return TransformCameraPlanarInput(strafe, forward);
        }

        /// <summary>优先 I/J/K/L（与 WASD 同向）；否则鼠标指向；否则保持上一帧炮口方向。</summary>
        private Vector2 ReadAimDirection()
        {
            float strafe = (Input.GetKey(KeyCode.L) ? 1f : 0f) - (Input.GetKey(KeyCode.J) ? 1f : 0f);
            float forward = (Input.GetKey(KeyCode.I) ? 1f : 0f) - (Input.GetKey(KeyCode.K) ? 1f : 0f);
            if (Mathf.Abs(strafe) + Mathf.Abs(forward) > 0.01f)
            {
                cachedAimDirection = TransformCameraPlanarInput(strafe, forward);
                return cachedAimDirection;
            }

            Camera cam = Camera.main;
            if (cam != null)
            {
                Ray ray = cam.ScreenPointToRay(Input.mousePosition);
                var ground = new Plane(Vector3.up, Vector3.zero);
                if (ground.Raycast(ray, out float dist))
                {
                    Vector3 hit = ray.GetPoint(dist);
                    float dx = hit.x - cachedPlayerTankPlanarPos.x;
                    float dz = hit.z - cachedPlayerTankPlanarPos.y;
                    var m = new Vector2(dx, dz);
                    if (m.Length > 0.05f)
                    {
                        cachedAimDirection = m.Normalized;
                        return cachedAimDirection;
                    }
                }
            }

            return cachedAimDirection;
        }

        private void UpdateGameState()
        {
            if (gameCore == null) return;

            var snapshot = gameCore.GetGameState();
            if (snapshot == null) return;

            RefreshCachedPlayerPlanarPosition(snapshot);
            OnGameStateChanged?.Invoke(snapshot);
            CheckGameState();
        }

        private void RefreshCachedPlayerPlanarPosition(GameSnapshot snapshot)
        {
            if (snapshot.tanks == null || playerId == 0) return;
            float scale = 0.22f;
            var gm = FindObjectOfType<GameManager>();
            if (gm != null)
                scale = gm.WorldDisplayScale;

            for (int i = 0; i < snapshot.tanks.Length; i++)
            {
                if (snapshot.tanks[i].playerId == playerId)
                {
                    Vector2 native = snapshot.tanks[i].position;
                    cachedPlayerTankPlanarPos = new Vector2(native.x * scale, native.y * scale);
                    return;
                }
            }
        }

        private void CheckGameState()
        {
            if (gameCore == null || gameOverNotified) return;
            if (!gameCore.IsGameOver()) return;

            gameOverNotified = true;
            allowLocalInput = false;
            Faction winner = gameCore.GetWinner();
            Debug.Log($"Game Over! Winner: {winner}");
            OnGameEnded?.Invoke(winner);
        }
    }
}
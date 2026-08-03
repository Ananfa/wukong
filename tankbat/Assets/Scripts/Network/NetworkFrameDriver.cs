using System;
using System.Collections.Generic;
using UnityEngine;
using Wukong.Pb;

namespace TankBattle.Network
{
    /// <summary>
    /// Online frame authority: apply server Snapshot/FrameSync to local GameCore;
    /// upload local input for GetFrame()+1. Does not tick simulation from Unity deltaTime.
    /// </summary>
    public class NetworkFrameDriver
    {
        private readonly TankBattleNative.GameCore _gameCore;
        private readonly KcpBattleClient _kcp;
        private readonly Func<PlayerInput?> _buildLocalInput;
        private bool _simulationReady;
        private uint _localPlayerId;
        private ulong _localRoleId;
        private Faction _localFaction = Faction.Soviet;
        private bool _startedNotified;
        // Snapshot 延迟到达时暂存 FrameSync，避免先丢帧再 mismatch 永久卡死
        private readonly Queue<BattleFrameSync> _pendingFrameSyncs = new Queue<BattleFrameSync>();
        private const int MaxPendingFrameSyncs = 90;
        private const uint CompareDumpFrame = 600; // 30Hz → 第 20 秒
        private bool _compareDumpedAt600;

        public bool IsReady => _simulationReady;
        public uint LocalPlayerId => _localPlayerId;
        public Faction LocalFaction => _localFaction;

        public event Action OnSimulationStarted;
        public event Action OnSimulationEnded;

        public NetworkFrameDriver(
            TankBattleNative.GameCore gameCore,
            KcpBattleClient kcp,
            Func<PlayerInput?> buildLocalInput)
        {
            _gameCore = gameCore ?? throw new ArgumentNullException(nameof(gameCore));
            _kcp = kcp ?? throw new ArgumentNullException(nameof(kcp));
            _buildLocalInput = buildLocalInput;
            _kcp.OnSnapshot += HandleSnapshot;
            _kcp.OnFrameSync += HandleFrameSync;
        }

        public void Dispose()
        {
            _kcp.OnSnapshot -= HandleSnapshot;
            _kcp.OnFrameSync -= HandleFrameSync;
        }

        public void Reset()
        {
            _simulationReady = false;
            _localPlayerId = 0;
            _startedNotified = false;
            _pendingFrameSyncs.Clear();
            _compareDumpedAt600 = false;
        }

        /// <summary>Call each Unity frame while online: pump KCP + upload next-frame input.</summary>
        public void Tick()
        {
            _kcp.Update();

            if (!_simulationReady || !_kcp.IsAuthed)
                return;

            PlayerInput? local = _buildLocalInput?.Invoke();
            if (!local.HasValue)
                return;

            PlayerInput input = local.Value;
            input.playerId = _localPlayerId;
            input.frame = _gameCore.GetFrame() + 1;
            _kcp.UploadInput(input);
        }

        private void HandleSnapshot(BattleRoomSnapshot snap)
        {
            if (snap.RoomState == BattleRoomState.Ended)
            {
                OnSimulationEnded?.Invoke();
                return;
            }

            _localRoleId = _kcp.Session != null ? _kcp.Session.RoleId : 0;
            _localPlayerId = 0;
            _localFaction = Faction.Soviet;

            int tankCount = snap.Tanks != null ? snap.Tanks.Count : 0;
            // 中途加入若无 tanks，绝不能 Bootstrap/StartGameAIOnly（会在出生点重开一局，德军杀进苏联老家）
            bool midJoin = snap.LogicFrame > 0 || snap.RoomState == BattleRoomState.Running;
            if (tankCount == 0)
            {
                if (midJoin)
                {
                    Debug.LogError(
                        $"NetworkFrameDriver: mid-join snapshot missing tanks frame={snap.LogicFrame} " +
                        $"state={snap.RoomState} players={snap.Players.Count}. " +
                        "Refuse Bootstrap. Rebuild/redeploy battle server with full Snapshot export.");
                    return;
                }
                BootstrapFromPlayerList(snap);
            }
            else if (!ApplyFullLogicSnapshot(snap))
            {
                Debug.LogError("NetworkFrameDriver: ApplyLogicSnapshot failed");
                return;
            }

            BindLocalFromSnapshotPlayers(snap);

            _simulationReady = true;
            LogSnapshotTankPositions(snap, "net");
            LogNativeTankPositions("after-apply");
            Debug.Log(
                $"NetworkFrameDriver: snapshot applied frame={_gameCore.GetFrame()} " +
                $"localPlayerId={_localPlayerId} tanks={tankCount} players={snap.Players.Count} " +
                $"aiMem={(snap.AiMemories != null ? snap.AiMemories.Count : 0)}");

            DrainPendingFrameSyncs();

            if (!_startedNotified)
            {
                _startedNotified = true;
                OnSimulationStarted?.Invoke();
            }
        }

        private void DrainPendingFrameSyncs()
        {
            while (_pendingFrameSyncs.Count > 0)
            {
                BattleFrameSync fr = _pendingFrameSyncs.Dequeue();
                if (fr.FrameIndex <= _gameCore.GetFrame())
                    continue;
                ApplyFrameSync(fr);
            }
        }

        private void BootstrapFromPlayerList(BattleRoomSnapshot snap)
        {
            Debug.LogWarning(
                "NetworkFrameDriver: BootstrapFromPlayerList (StartGameAIOnly) — only for empty brand-new room");
            _gameCore.Reset();
            _gameCore.Initialize(8);
            uint slots = snap.SlotsPerFaction > 0 ? snap.SlotsPerFaction : 2u;
            _gameCore.SetSlotsPerFaction(slots);
            _gameCore.SetRandomSeed(snap.RandomSeed == 0 ? 1u : snap.RandomSeed);
            _gameCore.StartGameAIOnly();

            var players = new List<BattleSnapshotPlayer>(snap.Players);
            players.Sort((a, b) => a.PlayerId.CompareTo(b.PlayerId));
            for (int i = 0; i < players.Count; i++)
            {
                var p = players[i];
                Faction faction = (Faction)Mathf.Clamp(p.Faction, 0, 3);
                uint tankId;
                uint id = _gameCore.PossessAITank($"role_{p.RoleId}", faction, out tankId);
                if (id == 0)
                    Debug.LogError($"NetworkFrameDriver: PossessAITank failed role={p.RoleId}");
            }
        }

        private static void LogSnapshotTankPositions(BattleRoomSnapshot snap, string tag)
        {
            if (snap.Tanks == null) return;
            for (int i = 0; i < snap.Tanks.Count && i < 12; i++)
            {
                var t = snap.Tanks[i];
                Debug.Log(
                    $"NetworkFrameDriver[{tag}] tank id={t.Id} fac={t.Faction} player={t.PlayerId} " +
                    $"pos=({t.PosX},{t.PosY}) hp={t.Hp} alive={t.IsAlive}");
            }
        }

        private void LogNativeTankPositions(string tag)
        {
            var gs = _gameCore.GetGameState();
            if (gs?.tanks == null) return;
            for (int i = 0; i < gs.tanks.Length && i < 12; i++)
            {
                var t = gs.tanks[i];
                Debug.Log(
                    $"NetworkFrameDriver[{tag}] tank id={t.id} fac={t.faction} player={t.playerId} " +
                    $"world=({t.position.x:F1},{t.position.y:F1}) hp={t.hp:F0}");
            }
        }

        private bool ApplyFullLogicSnapshot(BattleRoomSnapshot snap)
        {
            _gameCore.Reset();
            _gameCore.Initialize(8);

            var tanks = new TankBattleNative.TB_TankLogicSnapshot[snap.Tanks.Count];
            for (int i = 0; i < snap.Tanks.Count; i++)
            {
                var t = snap.Tanks[i];
                tanks[i] = new TankBattleNative.TB_TankLogicSnapshot
                {
                    id = t.Id,
                    playerId = t.PlayerId,
                    faction = t.Faction,
                    type = t.Type,
                    posX = t.PosX,
                    posY = t.PosY,
                    velX = t.VelX,
                    velY = t.VelY,
                    rotation = t.Rotation,
                    turretRotation = t.TurretRotation,
                    hp = t.Hp,
                    maxHp = t.MaxHp,
                    shieldFrames = t.ShieldFrames,
                    speedBoostFrames = t.SpeedBoostFrames,
                    rapidFireFrames = t.RapidFireFrames,
                    abilityCooldownFrames = t.AbilityCooldownFrames,
                    reloadFrames = t.ReloadFrames,
                    reloadDurationFrames = t.ReloadDurationFrames,
                    recoilVelX = t.RecoilVelX,
                    recoilVelY = t.RecoilVelY,
                    lockedTargetId = t.LockedTargetId,
                    aiMoveMode = t.AiMoveMode,
                    respawnFrames = t.RespawnFrames,
                    spawnProtectionFrames = t.SpawnProtectionFrames,
                    chargedShot = t.ChargedShot ? (byte)1 : (byte)0,
                    isPlayer = t.IsPlayer ? (byte)1 : (byte)0,
                    // proto3 默认 false：hp>0 视为存活，防止误删 AI 后 GenerateAITanks 刷在出生区
                    isAlive = (t.IsAlive || t.Hp > 0) ? (byte)1 : (byte)0,
                    pad = 0
                };
            }

            if (snap.Tanks.Count > 0)
            {
                var t0 = snap.Tanks[0];
                Debug.Log(
                    $"NetworkFrameDriver: snapshot tank[0] id={t0.Id} player={t0.PlayerId} " +
                    $"pos=({t0.PosX},{t0.PosY}) hp={t0.Hp}/{t0.MaxHp} frame={snap.LogicFrame} " +
                    $"tanks={snap.Tanks.Count}");
            }

            var bullets = new TankBattleNative.TB_BulletLogicSnapshot[snap.Bullets.Count];
            for (int i = 0; i < snap.Bullets.Count; i++)
            {
                var b = snap.Bullets[i];
                var tb = new TankBattleNative.TB_BulletLogicSnapshot
                {
                    id = b.Id,
                    ownerId = b.OwnerId,
                    posX = b.PosX,
                    posY = b.PosY,
                    velX = b.VelX,
                    velY = b.VelY,
                    damage = b.Damage,
                    lifeFrames = b.LifeFrames,
                    penetrating = b.Penetrating ? (byte)1 : (byte)0,
                    pad0 = 0,
                    pad1 = 0,
                    pad2 = 0,
                    damagedCount = (uint)Mathf.Min(b.DamagedTankIds.Count, 8)
                };
                if (b.DamagedTankIds.Count > 0) tb.d0 = b.DamagedTankIds[0];
                if (b.DamagedTankIds.Count > 1) tb.d1 = b.DamagedTankIds[1];
                if (b.DamagedTankIds.Count > 2) tb.d2 = b.DamagedTankIds[2];
                if (b.DamagedTankIds.Count > 3) tb.d3 = b.DamagedTankIds[3];
                if (b.DamagedTankIds.Count > 4) tb.d4 = b.DamagedTankIds[4];
                if (b.DamagedTankIds.Count > 5) tb.d5 = b.DamagedTankIds[5];
                if (b.DamagedTankIds.Count > 6) tb.d6 = b.DamagedTankIds[6];
                if (b.DamagedTankIds.Count > 7) tb.d7 = b.DamagedTankIds[7];
                bullets[i] = tb;
            }

            var players = new List<TankBattleNative.PlayerLogicSnap>();
            for (int i = 0; i < snap.Players.Count; i++)
            {
                var p = snap.Players[i];
                players.Add(new TankBattleNative.PlayerLogicSnap
                {
                    id = p.PlayerId,
                    name = $"role_{p.RoleId}",
                    faction = p.Faction,
                    kills = 0,
                    score = 0,
                    isConnected = true
                });
            }

            var aiMemories = new TankBattleNative.TB_AiMemorySnapshot[snap.AiMemories != null ? snap.AiMemories.Count : 0];
            for (int i = 0; i < aiMemories.Length; i++)
            {
                var src = snap.AiMemories[i];
                var m = new TankBattleNative.TB_AiMemorySnapshot
                {
                    tankId = src.TankId,
                    wanderHeading = src.WanderHeading,
                    strafeSign = src.StrafeSign,
                    strafeSwitchFrames = src.StrafeSwitchFrames,
                    wanderGoalSerial = src.WanderGoalSerial,
                    pathGoalX = src.PathGoalX,
                    pathGoalY = src.PathGoalY,
                    pathTargetId = src.PathTargetId,
                    pathMoveMode = src.PathMoveMode,
                    pathRecalcFrames = src.PathRecalcFrames,
                    wanderPathGoalX = src.WanderPathGoalX,
                    wanderPathGoalY = src.WanderPathGoalY,
                    wanderPathFrames = src.WanderPathFrames,
                    pathWaypointIndex = src.PathWaypointIndex,
                    pathWaypointCoords = new int[64]
                };
                int n = src.PathWaypointCoords != null ? src.PathWaypointCoords.Count : 0;
                if (n > 64) n = 64;
                m.waypointCoordCount = (uint)n;
                for (int c = 0; c < n; c++)
                    m.pathWaypointCoords[c] = src.PathWaypointCoords[c];
                aiMemories[i] = m;
            }

            uint[] kills = new uint[4];
            uint[] deaths = new uint[4];
            for (int i = 0; i < 4 && i < snap.FactionKills.Count; i++)
                kills[i] = snap.FactionKills[i];
            for (int i = 0; i < 4 && i < snap.FactionDeaths.Count; i++)
                deaths[i] = snap.FactionDeaths[i];

            Debug.Log(
                $"NetworkFrameDriver: apply snapshot tanks={snap.Tanks.Count} aiMem={aiMemories.Length} " +
                $"frame={snap.LogicFrame}");

            return _gameCore.ApplyLogicSnapshot(
                snap.LogicFrame,
                snap.GameState,
                snap.RandomSeed == 0 ? 1u : snap.RandomSeed,
                snap.SlotsPerFaction > 0 ? snap.SlotsPerFaction : 2u,
                snap.NextPlayerId > 0 ? snap.NextPlayerId : 1u,
                snap.NextTankId > 0 ? snap.NextTankId : 1u,
                snap.NextBulletId > 0 ? snap.NextBulletId : 1u,
                kills, deaths,
                tanks, bullets, players, aiMemories);
        }

        private void BindLocalFromSnapshotPlayers(BattleRoomSnapshot snap)
        {
            for (int i = 0; i < snap.Players.Count; i++)
            {
                var p = snap.Players[i];
                if (p.RoleId == _localRoleId)
                {
                    _localPlayerId = p.PlayerId;
                    _localFaction = (Faction)Mathf.Clamp(p.Faction, 0, 3);
                    return;
                }
            }
            if (snap.Players.Count > 0)
            {
                _localPlayerId = snap.Players[0].PlayerId;
                _localFaction = (Faction)Mathf.Clamp(snap.Players[0].Faction, 0, 3);
                Debug.LogWarning("NetworkFrameDriver: role not in snapshot; using first player");
            }
        }

        private void HandleFrameSync(BattleFrameSync fr)
        {
            if (!_simulationReady)
            {
                if (_pendingFrameSyncs.Count >= MaxPendingFrameSyncs)
                    _pendingFrameSyncs.Dequeue();
                _pendingFrameSyncs.Enqueue(fr);
                return;
            }

            ApplyFrameSync(fr);
        }

        private void ApplyFrameSync(BattleFrameSync fr)
        {
            uint expected = _gameCore.GetFrame() + 1;
            if (fr.FrameIndex != expected)
            {
                Debug.LogWarning(
                    $"NetworkFrameDriver: frame mismatch got={fr.FrameIndex} expected={expected}");
                return;
            }

            // 控制事件：幂等 Possess/Release（AI 记忆由 Snapshot.ai_memories 对齐，勿 ClearAi）
            if (fr.ControlEvents != null && fr.ControlEvents.Count > 0)
            {
                for (int i = 0; i < fr.ControlEvents.Count; i++)
                {
                    var ev = fr.ControlEvents[i];
                    if (ev.Type == BattleControlEventType.BattleControlPossess)
                    {
                        Faction faction = (Faction)Mathf.Clamp(ev.Faction, 0, 3);
                        if (!_gameCore.ApplyPossess(ev.PlayerId, ev.Name ?? $"role_{ev.RoleId}", faction, ev.TankId))
                            Debug.LogWarning($"NetworkFrameDriver: ApplyPossess failed player={ev.PlayerId} tank={ev.TankId}");
                        if (ev.RoleId == _localRoleId)
                        {
                            _localPlayerId = ev.PlayerId;
                            _localFaction = faction;
                        }
                        Debug.Log(
                            $"NetworkFrameDriver: Possess playerId={ev.PlayerId} tankId={ev.TankId} " +
                            $"faction={faction} role={ev.RoleId}");
                    }
                    else if (ev.Type == BattleControlEventType.BattleControlRelease)
                    {
                        _gameCore.ApplyRelease(ev.PlayerId);
                        if (ev.PlayerId == _localPlayerId)
                            _localPlayerId = 0;
                    }
                }
            }

            PlayerInput[] inputs = new PlayerInput[fr.Inputs.Count];
            for (int i = 0; i < fr.Inputs.Count; i++)
            {
                var src = fr.Inputs[i];
                inputs[i] = new PlayerInput
                {
                    playerId = src.PlayerId,
                    frame = fr.FrameIndex,
                    moveX = (short)Mathf.Clamp(src.MoveX, short.MinValue, short.MaxValue),
                    moveY = (short)Mathf.Clamp(src.MoveY, short.MinValue, short.MaxValue),
                    aimX = (short)Mathf.Clamp(src.AimX, short.MinValue, short.MaxValue),
                    aimY = (short)Mathf.Clamp(src.AimY, short.MinValue, short.MaxValue),
                    fire = src.Fire,
                    useAbility = src.UseAbility,
                    timestamp = src.Timestamp
                };
            }

            if (!_gameCore.SetFrameInputs(fr.FrameIndex, inputs))
            {
                Debug.LogWarning($"NetworkFrameDriver: SetFrameInputs failed frame={fr.FrameIndex}");
                return;
            }

            _gameCore.AdvanceSimulation();
            MaybeDumpCompareSnapshot();
        }

        private void MaybeDumpCompareSnapshot()
        {
            if (_compareDumpedAt600)
                return;
            if (_gameCore.GetFrame() != CompareDumpFrame)
                return;
            _compareDumpedAt600 = true;
            try
            {
                string text = _gameCore.FormatCompareSnapshot("CLIENT");
                Debug.Log(text);
                string path = System.IO.Path.Combine(
                    Application.persistentDataPath, "compare_snap_frame_600_client.txt");
                System.IO.File.WriteAllText(path, text);
                Debug.Log($"[CompareSnap] wrote {path}");
            }
            catch (Exception ex)
            {
                Debug.LogWarning("CompareSnap dump failed: " + ex.Message);
            }
        }
    }
}

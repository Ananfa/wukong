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

            if (snap.LogicFrame != 0)
            {
                Debug.LogWarning(
                    $"NetworkFrameDriver: snapshot logic_frame={snap.LogicFrame} > 0; " +
                    "mid-join without full state not supported yet.");
            }

            _gameCore.Reset();
            _gameCore.Initialize(8);
            _gameCore.SetRandomSeed(snap.RandomSeed == 0 ? 1u : snap.RandomSeed);

            _localRoleId = _kcp.Session != null ? _kcp.Session.RoleId : 0;
            _localPlayerId = 0;
            _localFaction = Faction.Soviet;

            var players = new List<BattleSnapshotPlayer>(snap.Players);
            players.Sort((a, b) => a.PlayerId.CompareTo(b.PlayerId));

            if (players.Count == 0)
            {
                _localPlayerId = _gameCore.AddPlayer("local", Faction.Soviet);
                _localFaction = Faction.Soviet;
            }
            else
            {
                for (int i = 0; i < players.Count; i++)
                {
                    var p = players[i];
                    Faction faction = (Faction)Mathf.Clamp(p.Faction, 0, 3);
                    uint id = _gameCore.AddPlayer($"role_{p.RoleId}", faction);
                    if (id == 0)
                    {
                        Debug.LogError($"NetworkFrameDriver: AddPlayer failed for role {p.RoleId}");
                        continue;
                    }
                    if (id != p.PlayerId)
                    {
                        Debug.LogWarning(
                            $"NetworkFrameDriver: local playerId {id} != server {p.PlayerId} for role {p.RoleId}");
                    }
                    if (p.RoleId == _localRoleId)
                    {
                        _localPlayerId = id;
                        _localFaction = faction;
                    }
                }
            }

            if (_localPlayerId == 0 && players.Count > 0)
            {
                _localPlayerId = players[0].PlayerId;
                _localFaction = (Faction)Mathf.Clamp(players[0].Faction, 0, 3);
                Debug.LogWarning("NetworkFrameDriver: role not in snapshot; using first server playerId");
            }

            _gameCore.StartGame();
            _simulationReady = true;
            Debug.Log(
                $"NetworkFrameDriver: StartGame seed={snap.RandomSeed} frame={_gameCore.GetFrame()} " +
                $"localPlayerId={_localPlayerId} players={players.Count}");
            OnSimulationStarted?.Invoke();
        }

        private void HandleFrameSync(BattleFrameSync fr)
        {
            if (!_simulationReady)
                return;

            uint expected = _gameCore.GetFrame() + 1;
            if (fr.FrameIndex != expected)
            {
                Debug.LogWarning(
                    $"NetworkFrameDriver: frame mismatch got={fr.FrameIndex} expected={expected}");
                return;
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
        }
    }
}

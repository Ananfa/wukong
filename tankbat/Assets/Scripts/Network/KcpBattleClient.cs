using System;
using System.Threading.Tasks;
using Corpc;
using Google.Protobuf;
using UnityEngine;
using Wukong.Pb;

namespace TankBattle.Network
{
    /// <summary>
    /// Battle KCP client (based on libcorpc KcpMessageClient).
    /// Protocol: UDP handshake → KCP → 2101 Auth → 2104 Snapshot / 2105 FrameSync; upload 2106.
    /// </summary>
    public class KcpBattleClient
    {
        private KcpMessageClient _client;
        private readonly System.Random _rnd = new System.Random();
        private int _localPort;
        private bool _authed;
        private BattleEnterSession _session;

        public bool IsRunning => _client != null && _client.Running;
        public bool IsAuthed => _authed;
        public BattleEnterSession Session => _session;

        public event Action OnConnected;
        public event Action OnDisconnected;
        public event Action<BattleRoomSnapshot> OnSnapshot;
        public event Action<BattleFrameSync> OnFrameSync;

        public async Task<bool> ConnectAndAuthAsync(BattleEnterSession session)
        {
            if (session == null || string.IsNullOrEmpty(session.KcpHost) || session.KcpPort <= 0)
            {
                Debug.LogError("KcpBattleClient: invalid BattleEnterSession");
                return false;
            }

            Disconnect();
            _session = session;
            _authed = false;

            _client = new KcpMessageClient(session.KcpHost, session.KcpPort, true, true, true, true);
            _client.Register(BattleMessageIds.Snapshot, BattleRoomSnapshot.Parser, HandleSnapshot);
            _client.Register(BattleMessageIds.FrameSync, BattleFrameSync.Parser, HandleFrameSync);
            _client.Register(Constants.CORPC_MSG_TYPE_CONNECT, null, HandleConnect);
            _client.Register(Constants.CORPC_MSG_TYPE_DISCONNECT, null, HandleDisconnect);

            _localPort = 20000 + _rnd.Next(10000);
            for (int attempt = 0; attempt < 8; attempt++)
            {
                _client.localPort_ = _localPort;
                bool ok = await _client.Start();
                if (ok)
                    return true;
                _localPort = 20000 + (_localPort - 20000 + 1) % 10000;
                await Task.Delay(200);
            }

            Debug.LogError("KcpBattleClient: failed to connect");
            return false;
        }

        public void Update()
        {
            if (_client != null && _client.Running)
                _client.Update();
        }

        public void SendAuth()
        {
            if (_client == null || !_client.Running || _session == null)
                return;

            var auth = new BattleKcpAuth
            {
                RoomId = _session.RoomId,
                RoleId = _session.RoleId,
                SessionToken = _session.SessionToken ?? ""
            };
            _client.Send(BattleMessageIds.Auth, 0, auth, false);
            Debug.Log($"KcpBattleClient: sent AUTH room={_session.RoomId} role={_session.RoleId}");
        }

        public void UploadInput(PlayerInput input)
        {
            if (_client == null || !_client.Running || _session == null || !_authed)
                return;

            var msg = new BattleKcpInputUpload
            {
                RoomId = _session.RoomId,
                RoleId = _session.RoleId,
                Frame = input.frame,
                MoveX = input.moveX,
                MoveY = input.moveY,
                AimX = input.aimX,
                AimY = input.aimY,
                Fire = input.fire,
                UseAbility = input.useAbility,
                Timestamp = input.timestamp
            };
            _client.Send(BattleMessageIds.InputUpload, 0, msg, false);
        }

        public void LeaveRoom()
        {
            if (_client == null || !_client.Running || _session == null)
                return;

            var leave = new BattleKcpLeaveRoom
            {
                RoomId = _session.RoomId,
                RoleId = _session.RoleId,
                SessionToken = _session.SessionToken ?? ""
            };
            _client.Send(BattleMessageIds.LeaveRoom, 0, leave, false);
        }

        public void Disconnect()
        {
            _authed = false;
            if (_client != null)
            {
                try
                {
                    if (_client.Running)
                        _client.Close();
                }
                catch (Exception ex)
                {
                    Debug.LogWarning($"KcpBattleClient.Disconnect: {ex.Message}");
                }
                _client = null;
            }
        }

        private void HandleConnect(int type, IMessage msg)
        {
            Debug.Log("KcpBattleClient: CONNECT");
            OnConnected?.Invoke();
            SendAuth();
        }

        private void HandleDisconnect(int type, IMessage msg)
        {
            Debug.Log("KcpBattleClient: DISCONNECT");
            _authed = false;
            OnDisconnected?.Invoke();
        }

        private void HandleSnapshot(int type, IMessage msg)
        {
            var snap = msg as BattleRoomSnapshot;
            if (snap == null) return;
            _authed = true;
            OnSnapshot?.Invoke(snap);
        }

        private void HandleFrameSync(int type, IMessage msg)
        {
            var fr = msg as BattleFrameSync;
            if (fr == null) return;
            OnFrameSync?.Invoke(fr);
        }
    }

    [Serializable]
    public class BattleEnterSession
    {
        public string KcpHost = "127.0.0.1";
        public int KcpPort = 19001;
        public ulong RoomId;
        public ulong RoleId = 1;
        public int BattleServerId;
        public string SessionToken = "";
    }
}

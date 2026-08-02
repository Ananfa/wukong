using System;
using System.Text;
using System.Threading.Tasks;
using Corpc;
using Google.Protobuf;
using UnityEngine;
using Wukong.Pb;

namespace TankBattle.Network
{
    /// <summary>
    /// Gateway/Front TCP session: Auth → ENTERGAME/ENTERLOBBY → StartBattle → BATTLE_ENTER.
    /// </summary>
    public class GatewaySession
    {
        private TcpMessageClient _client;
        private string _cipher;
        private ushort _sendTag = 1;
        private TaskCompletionSource<bool> _enterLobbyTcs;
        private TaskCompletionSource<BattleEnterInfo> _battleEnterTcs;

        public bool IsRunning => _client != null && _client.Running;
        public bool InLobby { get; private set; }

        public event Action OnConnected;
        public event Action OnDisconnected;
        public event Action OnEnterGame;
        public event Action OnEnterLobby;
        public event Action<BattleEnterInfo> OnBattleEnter;
        public event Action<int> OnError;

        public async Task<bool> ConnectAndAuthAsync(
            string host, int port, ulong userId, string gToken, uint gateId, string cipher)
        {
            Disconnect();
            InLobby = false;
            _cipher = cipher ?? Guid.NewGuid().ToString("N");

            _client = new TcpMessageClient(host, port, true, true, true, true);
            _client.Crypter = new SimpleXORCrypter(Encoding.UTF8.GetBytes(_cipher));
            _client.Register(WukongMessageIds.S2C_EnterGame, DataFragments.Parser, HandleEnterGame);
            _client.Register(WukongMessageIds.S2C_Reconnected, null, HandleReconnected);
            _client.Register(WukongMessageIds.S2C_EnterLobby, Int32Value.Parser, HandleEnterLobby);
            _client.Register(WukongMessageIds.S2C_BattleEnter, BattleEnterInfo.Parser, HandleBattleEnter);
            _client.Register(WukongMessageIds.S2C_Error, Int32Value.Parser, HandleError);
            _client.Register(Constants.CORPC_MSG_TYPE_CONNECT, null, HandleConnect);
            _client.Register(Constants.CORPC_MSG_TYPE_DISCONNECT, null, HandleDisconnect);

            bool ok = await _client.Start();
            if (!ok)
            {
                Debug.LogError("GatewaySession: TCP connect failed");
                return false;
            }

            var auth = new AuthRequest
            {
                UserId = userId,
                Token = gToken ?? "",
                Cipher = _cipher,
                RecvSerial = 0,
                GateId = gateId
            };
            // First auth must not use crypter (matches demo client)
            _client.Send(WukongMessageIds.C2S_Auth, 0, auth, false);
            Debug.Log($"GatewaySession: AUTH sent userId={userId} gateId={gateId}");
            return true;
        }

        public void Update()
        {
            if (_client != null && _client.Running)
                _client.Update();
        }

        public Task WaitEnterLobbyAsync()
        {
            if (InLobby)
                return Task.CompletedTask;
            _enterLobbyTcs = new TaskCompletionSource<bool>();
            return _enterLobbyTcs.Task;
        }

        public Task<BattleEnterInfo> RequestStartBattleAsync(uint battleDefId, int faction = 0)
        {
            if (_client == null || !_client.Running)
                throw new InvalidOperationException("GatewaySession not connected");

            _battleEnterTcs = new TaskCompletionSource<BattleEnterInfo>();
            var req = new StartBattleRequest
            {
                BattleDefId = battleDefId,
                Faction = faction
            };
            _client.Send(WukongMessageIds.C2S_StartBattle, ++_sendTag, req, true);
            Debug.Log($"GatewaySession: START_BATTLE battleDefId={battleDefId} faction={faction}");
            return _battleEnterTcs.Task;
        }

        public void Disconnect()
        {
            InLobby = false;
            if (_client != null)
            {
                try
                {
                    if (_client.Running)
                        _client.Close();
                }
                catch (Exception ex)
                {
                    Debug.LogWarning("GatewaySession.Disconnect: " + ex.Message);
                }
                _client = null;
            }
            _enterLobbyTcs?.TrySetCanceled();
            _battleEnterTcs?.TrySetCanceled();
            _enterLobbyTcs = null;
            _battleEnterTcs = null;
        }

        private void HandleConnect(int type, IMessage msg)
        {
            OnConnected?.Invoke();
        }

        private void HandleDisconnect(int type, IMessage msg)
        {
            InLobby = false;
            OnDisconnected?.Invoke();
        }

        private void HandleEnterGame(int type, IMessage msg)
        {
            Debug.Log("GatewaySession: ENTERGAME");
            OnEnterGame?.Invoke();
        }

        private void HandleReconnected(int type, IMessage msg)
        {
            Debug.Log("GatewaySession: RECONNECTED");
            OnEnterGame?.Invoke();
        }

        private void HandleEnterLobby(int type, IMessage msg)
        {
            Debug.Log("GatewaySession: ENTERLOBBY");
            InLobby = true;
            OnEnterLobby?.Invoke();
            _enterLobbyTcs?.TrySetResult(true);
        }

        private void HandleBattleEnter(int type, IMessage msg)
        {
            var enter = msg as BattleEnterInfo;
            if (enter == null) return;
            Debug.Log($"GatewaySession: BATTLE_ENTER host={enter.KcpHost}:{enter.KcpPort} room={enter.RoomId}");
            OnBattleEnter?.Invoke(enter);
            _battleEnterTcs?.TrySetResult(enter);
        }

        private void HandleError(int type, IMessage msg)
        {
            var err = msg as Int32Value;
            int code = err != null ? err.Value : -1;
            Debug.LogError("GatewaySession: S2C_ERROR code=" + code);
            OnError?.Invoke(code);
            _battleEnterTcs?.TrySetException(new Exception("battle enter error " + code));
        }
    }
}

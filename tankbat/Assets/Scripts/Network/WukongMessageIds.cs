namespace TankBattle.Network
{
    /// <summary>Mirrors wukong/share/const.h client-facing message IDs.</summary>
    public static class WukongMessageIds
    {
        public const int ServerTypeLobby = 5;

        public const int C2S_Auth = 1;
        public const int C2S_Echo = (ServerTypeLobby << 16) | 1;
        public const int C2S_StartBattle = (ServerTypeLobby << 16) | 10;
        public const int C2S_LeaveGame = (ServerTypeLobby << 16) | 11;

        public const int S2C_EnterGame = 2;
        public const int S2C_Reconnected = 3;
        public const int S2C_Echo = 1000;
        public const int S2C_Error = 1001;
        public const int S2C_EnterLobby = 1101;
        public const int S2C_BattleEnter = 1200;
    }
}

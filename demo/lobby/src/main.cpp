
#include "lobby_server.h"
#include "lobby_config.h"
#include "game_center.h"
#include "gateway_client.h"
#include "record_client.h"
#include "share/const.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_LobbyServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    // 初始化全局资源
    g_GameCenter.init(GAME_SERVER_TYPE_LOBBY, g_LobbyConfig.getUpdatePeriod(), g_LobbyConfig.getCache().host.c_str(), g_LobbyConfig.getCache().port, g_LobbyConfig.getCache().dbIndex, g_LobbyConfig.getCache().maxConnect);
    // TODO: g_GameCenter.setDelegate(xxx);
    // TODO: 注册消息处理

    g_GatewayClient.init(g_LobbyServer.getRpcClient());
    g_RecordClient.init(g_LobbyServer.getRpcClient());

    // TODO: 注册其他GameClient
    //g_SceneClient.init(g_LobbyServer.getRpcClient());
    //g_LobbyServer.registerGameClient(&g_SceneClient);

    g_LobbyServer.run();
    return 0;
}
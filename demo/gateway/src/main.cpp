
#include "gateway_server.h"
#include "gateway_center.h"
#include "client_center.h"
#include "lobby_client.h"
#include "scene_client.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_GatewayServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    g_GatewayCenter.init();
    g_ClientCenter.init();

    g_LobbyClient.init(g_GatewayServer.getRpcClient());
    g_GatewayServer.registerGameClient(&g_LobbyClient);
    g_SceneClient.init(g_GatewayServer.getRpcClient());
    g_GatewayServer.registerGameClient(&g_SceneClient);

    g_GatewayServer.run();
    return 0;
}

#include "lobby_server.h"
#include "lobby_config.h"
#include "game_center.h"
#include "gateway_client.h"
#include "record_client.h"
#include "share/const.h"
#include "message_handler.h"

using namespace wukong;
using namespace demo;

int main(int argc, char * argv[]) {
    if (!g_LobbyServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    // 初始化全局资源
    g_GameCenter.init(GAME_SERVER_TYPE_LOBBY, g_LobbyConfig.getUpdatePeriod(), g_LobbyConfig.getCache().host.c_str(), g_LobbyConfig.getCache().port, g_LobbyConfig.getCache().dbIndex, g_LobbyConfig.getCache().maxConnect);
    
    GameDelegate delegate;
    delegate.createGameObject = [](UserId userId, RoleId roleId, ServerId serverId, uint32_t lToken, GameObjectManager* mgr, const std::string &data) -> std::shared_ptr<GameObject> {
        std::shared_ptr<GameObject> obj(new LobbyGameObject(userId, roleId, serverId, lToken, mgr));
        if (!obj->initData(data)) {
            ERROR_LOG("create game object failed because init data failed, role: %d\n", roleId);
            return nullptr;
        }

        return obj;
    };

    g_GameCenter.setDelegate(delegate);

    // 注册消息处理
    MessageHandler::registerMessages();

    g_GatewayClient.init(g_LobbyServer.getRpcClient());
    g_RecordClient.init(g_LobbyServer.getRpcClient());

    // TODO: 注册其他GameClient
    //g_SceneClient.init(g_LobbyServer.getRpcClient());
    //g_LobbyServer.registerGameClient(&g_SceneClient);

    g_LobbyServer.run();
    return 0;
}
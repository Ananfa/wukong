
#include "lobby_server.h"
#include "game_center.h"
#include "message_handler.h"

using namespace wukong;
using namespace demo;

int main(int argc, char * argv[]) {
    if (!g_LobbyServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

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

    // TODO: 策划配置加载及二次处理，配置动态加载

    g_LobbyServer.run();
    return 0;
}
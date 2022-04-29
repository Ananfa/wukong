
#include "lobby_server.h"
#include "game_delegate.h"
#include "lobby_delegate.h"
#include "message_handler.h"

using namespace wukong;
using namespace demo;

int main(int argc, char * argv[]) {
    if (!g_LobbyServer.init(argc, argv)) {
        ERROR_LOG("Can't init lobby server\n");
        return -1;
    }

    // TODO: 这里可以设置上层功能使用的redis lua脚本

    g_GameDelegate.setCreateGameObjectHandle([](UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager* mgr, const std::string &data) -> std::shared_ptr<GameObject> {
        std::shared_ptr<GameObject> obj(new LobbyGameObject(userId, roleId, serverId, lToken, mgr));
        if (!obj->initData(data)) {
            ERROR_LOG("create game object failed because init data failed, role: %d\n", roleId);
            return nullptr;
        }

        return obj;
    });

    g_LobbyDelegate.setGetTargetSceneIdHandle([](RoleId roleId) -> std::string {
        return ""; // 返回空串表示在大厅中加载游戏对象
    });

    // 注册消息处理
    MessageHandler::registerMessages();

    // TODO: 策划配置加载及二次处理，配置动态加载

    g_LobbyServer.run();
    return 0;
}
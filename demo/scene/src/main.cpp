
#include "scene_server.h"
#include "game_delegate.h"
#include "scene_delegate.h"
#include "message_handler.h"
#include "demo_scene.h"

using namespace wukong;
using namespace demo;

int main(int argc, char * argv[]) {
    if (!g_SceneServer.init(argc, argv)) {
        ERROR_LOG("Can't init scene server\n");
        return -1;
    }

    // TODO: 这里可以设置上层功能使用的redis lua脚本

    g_GameDelegate.setCreateGameObjectHandle([](UserId userId, RoleId roleId, ServerId serverId, const std::string &lToken, GameObjectManager* mgr, const std::string &data) -> std::shared_ptr<GameObject> {
        std::shared_ptr<GameObject> obj(new SceneGameObject(userId, roleId, serverId, lToken, mgr));
        if (!obj->initData(data)) {
            ERROR_LOG("create game object failed because init data failed, role: %d\n", roleId);
            return nullptr;
        }

        return obj;
    });

    g_SceneDelegate.setGetSceneTypeHandle([](uint32_t defId) -> SceneType {
        // TODO：根据配置ID返回场景类型，当前demo默认启动世界场景
        return SCENE_TYPE_GLOBAL;
    });

    g_SceneDelegate.setCreateSceneHandle([](uint32_t defId, SceneType sType, const std::string& sceneId, const std::string &sToken, SceneManager *manager) -> std::shared_ptr<Scene> {
        // 创建世界场景
        std::shared_ptr<Scene> scene(new DemoScene(defId, sType, sceneId, sToken, manager));
        return scene;
    });

    /** 若是队伍场景，需要设置获取队伍成员列表的delegate处理
    g_SceneDelegate.setGetMembersHandle([](const std::string& teamId) -> std::list<RoleId> {
        std::list<RoleId> members;
        // TODO：返回队伍成员
        return members;
    });
    */

    // 注册消息处理
    MessageHandler::registerMessages();

    // TODO: 策划配置加载及二次处理，配置动态加载

    g_SceneServer.run();
    return 0;
}
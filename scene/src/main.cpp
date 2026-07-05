
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

    // 注册消息处理
    MessageHandler::registerMessages();

    // TODO: 策划配置加载及二次处理，配置动态加载

    g_SceneServer.run();
    return 0;
}

#include "lobby_server.h"
#include "message_handler.h"

using namespace wukong;
using namespace demo;

int main(int argc, char * argv[]) {
    if (!g_LobbyServer.init(argc, argv)) {
        ERROR_LOG("Can't init lobby server\n");
        return -1;
    }

    // 注册消息处理
    MessageHandler::registerMessages();

    // TODO: 策划配置加载及二次处理，配置动态加载

    g_LobbyServer.run();
    return 0;
}
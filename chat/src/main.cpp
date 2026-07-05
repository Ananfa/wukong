
#include "chat_server.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_ChatServer.init(argc, argv)) {
        ERROR_LOG("Can't init chat server\n");
        return -1;
    }

    g_ChatServer.run();
    return 0;
}
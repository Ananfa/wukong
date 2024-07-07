
#include "front_server.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_FrontServer.init(argc, argv)) {
        ERROR_LOG("Can't init front server\n");
        return -1;
    }

    g_FrontServer.run();
    return 0;
}
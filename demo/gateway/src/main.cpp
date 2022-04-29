
#include "gateway_server.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_GatewayServer.init(argc, argv)) {
        ERROR_LOG("Can't init gateway server\n");
        return -1;
    }

    g_GatewayServer.run();
    return 0;
}
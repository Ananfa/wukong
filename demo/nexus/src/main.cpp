
#include "nexus_server.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_NexusServer.init(argc, argv)) {
        ERROR_LOG("Can't init nexus server\n");
        return -1;
    }

    g_NexusServer.run();
    return 0;
}
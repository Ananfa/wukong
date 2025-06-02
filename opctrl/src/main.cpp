
#include "opctrl_server.h"
#include "handler_mgr.h"

using namespace demo;
using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_OpctrlServer.init(argc, argv)) {
        ERROR_LOG("Can't init opctrl server\n");
        return -1;
    }

    HandlerMgr::init(g_OpctrlServer.getHttpServer());

    g_OpctrlServer.run();
    return 0;
}
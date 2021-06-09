
#include "record_server.h"
#include "record_center.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_RecordServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    g_RecordCenter.init();

    g_RecordServer.run();
    return 0;
}
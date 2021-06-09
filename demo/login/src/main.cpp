
#include "login_server.h"
#include "gateway_client.h"
#include "login_handler_mgr.h"
#include "demo_utils.h"

using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_LoginServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    g_GatewayClient.init(g_LoginServer.getRpcClient());

    LoginDelegate delegate;
    delegate.loginCheck = [](std::shared_ptr<RequestMessage> &request) -> bool {
        // TODO:
        return true;
    };

    delegate.createRole = [](std::shared_ptr<RequestMessage> &request, std::list<std::pair<std::string, std::string>>&pData) -> bool {
        // TODO:
        return false;
    };

    delegate.loadProfile = demo::DemoUtils::LoadProfile;
    
    delegate.makeProfile = demo::DemoUtils::MakeProfile;

    g_LoginHandlerMgr.init(g_LoginServer.getHttpServer(), delegate);

    g_LoginServer.run();
    return 0;
}
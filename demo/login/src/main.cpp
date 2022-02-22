
#include "login_server.h"
//#include "gateway_client.h"
#include "login_handler_mgr.h"
//#include "client_center.h"
#include "demo_db_utils.h"
#include "demo_utils.h"
#include "demo_role_builder.h"

using namespace demo;
using namespace wukong;

int main(int argc, char * argv[]) {
    if (!g_LoginServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    //g_GatewayClient.init(g_LoginServer.getRpcClient());

    LoginDelegate delegate;
    delegate.loginCheck = [](std::shared_ptr<RequestMessage> &request) -> bool {
        // TODO: 进行登录账号校验
        return true;
    };

    delegate.createRole = [](std::shared_ptr<RequestMessage> &request, std::list<std::pair<std::string, std::string>>&datas) -> bool {
        DemoRoleBuilder builder;

        if (!request->has("name")) {
            ERROR_LOG("Can't create role for no name\n");
            return false;
        }

        std::string name = (*request)["name"];
        builder.setName(name);

        builder.buildDatas(datas);
        return true;
    };

    delegate.loadProfile = demo::DemoDBUtils::LoadProfile;
    
    delegate.makeProfile = demo::DemoUtils::MakeProfile;

    g_LoginHandlerMgr.setDelegate(delegate);
    //g_ClientCenter.init();

    g_LoginServer.run();
    return 0;
}
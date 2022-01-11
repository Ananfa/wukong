
#include "record_server.h"
#include "record_center.h"
#include "demo_record_object.h"
#include "demo_utils.h"

using namespace wukong;
using namespace demo;

int main(int argc, char * argv[]) {
    if (!g_RecordServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    g_RecordCenter.init();
    
    RecordDelegate delegate;
    delegate.createRecordObject = [](RoleId roleId, ServerId serverId, uint32_t rToken, RecordObjectManager* mgr, std::list<std::pair<std::string, std::string>> datas) -> std::shared_ptr<RecordObject> {
        std::shared_ptr<RecordObject> obj(new DemoRecordObject(roleId, serverId, rToken, mgr));
        if (!obj->initData(datas)) {
        	ERROR_LOG("create record object failed because init data failed, role: %d\n", roleId);
            return nullptr;
        }

        return obj;
    };

    delegate.makeProfile = demo::DemoUtils::MakeProfile;

    g_RecordCenter.setDelegate(delegate);
    
    g_RecordServer.run();
    return 0;
}
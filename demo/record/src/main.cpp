
#include "record_server.h"
#include "record_delegate.h"
#include "demo_record_object.h"
#include "demo_utils.h"

using namespace wukong;
using namespace demo;

int main(int argc, char * argv[]) {
    if (!g_RecordServer.init(argc, argv)) {
        ERROR_LOG("Can't init login server\n");
        return -1;
    }

    g_RecordDelegate.setCreateRecordObjectHandle([](UserId userId, RoleId roleId, ServerId serverId, const std::string &rToken, RecordObjectManager* mgr, std::list<std::pair<std::string, std::string>> datas) -> std::shared_ptr<RecordObject> {
        std::shared_ptr<RecordObject> obj(new DemoRecordObject(userId, roleId, serverId, rToken, mgr));
        if (!obj->initData(datas)) {
            ERROR_LOG("create record object failed because init data failed, role: %d\n", roleId);
            return nullptr;
        }

        return obj;
    });
    g_RecordDelegate.setMakeProfileHandle(demo::DemoUtils::MakeProfile);

    g_RecordServer.run();
    return 0;
}
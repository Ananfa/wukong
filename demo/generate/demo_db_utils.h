// TODO：此文件通过工具生成

#ifndef demo_db_utils_h
#define demo_db_utils_h

#include "demo.pb.h"
#include "share/define.h"
#include <string>
#include <list>

namespace demo {
    
    class DemoDBUtils {
    public:
        static bool LoadProfile(RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas);
    };

}

#endif /* demo_db_utils_h */

// This file is generated. Don't edit it

#ifndef demo_utils_h
#define demo_utils_h

#include <string>
#include <list>
#include <map>
#include "share/define.h"

namespace demo {
    
    class DemoUtils {
    public:
        static bool LoadProfile(RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas);
        static void MakeProfile(const std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &pDatas);
    };

}

#endif /* demo_utils_h */

// TODO：此文件通过工具生成

#ifndef demo_db_utils_h
#define demo_db_utils_h

#include "demo.pb.h"
#include "corpc_redis.h"
#include "corpc_mysql.h"
#include "share/define.h"
#include <string>

using namespace corpc;

namespace demo {
    
    class DemoDBUtils {
    public:
        static bool LoadProfile(RedisConnectPool *cachePool, MysqlConnectPool *mysqlPool, const std::string &loadRoleSha1, const std::string &saveProfileSha1, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas);
    };

}

#endif /* demo_db_utils_h */

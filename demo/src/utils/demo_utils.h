// TODO：此文件通过工具生成

#ifndef demo_utils_h
#define wukong_proto_utils_h

#include "demo.pb.h"
#include "corpc_redis.h"
#include "corpc_mysql.h"
#include <string>

namespace demo {
    
    class DemoUtils {
    public:
    	static bool LoadProfile(RedisConnectPool *cachePool, MysqlConnectPool *mysqlPool, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas);
    };

}

#endif /* demo_utils_h */

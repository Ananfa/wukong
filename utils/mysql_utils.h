#ifndef wukong_mysql_utils_h
#define wukong_mysql_utils_h

#include <string>
#include <stdint.h>
#include "corpc_mysql.h"
#include "share/define.h"

namespace wukong {
    
    class MysqlUtils {
    public:
        static bool SaveUser(MYSQL *mysql, const std::string &account, UserId userId);
        static bool LoadRole(MYSQL *mysql, RoleId roleId, ServerId &serverId, std::string &data);
        static bool CreateRole(MYSQL *mysql, RoleId roleId, UserId userId, ServerId serverId, const std::string &data);
        static bool UpdateRole(MYSQL *mysql, RoleId roleId, const std::string &data);
    };

}

#endif /* wukong_mysql_utils_h */

#ifndef wukong_mysql_utils_h
#define wukong_mysql_utils_h

#include <string>
#include <vector>
#include <stdint.h>
#include "corpc_mysql.h"
#include "share/define.h"

namespace wukong {
    
    class MysqlUtils {
    public:
    	static bool LoadOrCreateUser(MYSQL *mysql, const std::string &account, UserId &userId, std::string &roleListStr);
    	static bool LoadRoleIds(MYSQL *mysql, UserId userId, std::string &roleListStr);
    	static bool UpdateRoleIds(MYSQL *mysql, UserId userId, const std::string &roleListStr, uint32_t roleNum);
        static bool LoadRole(MYSQL *mysql, RoleId roleId, UserId &userId, ServerId &serverId, std::string &data);
        static bool CreateRole(MYSQL *mysql, RoleId &roleId, UserId userId, ServerId serverId, const std::string &data);
        static bool UpdateRole(MYSQL *mysql, RoleId roleId, const std::string &data);
    	static bool CheckRole(MYSQL *mysql, RoleId roleId, UserId userId, ServerId serverId, bool &result);
    };

}

#endif /* wukong_mysql_utils_h */

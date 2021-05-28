#ifndef wukong_mysql_utils_h
#define wukong_mysql_utils_h

#include <string>
#include <stdint.h>
#include <hiredis.h>

namespace wukong {
    
    class MysqlUtils {
    public:
        static bool LoadRoleData(MYSQL *mysql, RoleId roleId, ServerId &serverId, std::string &data);
    };

}

#endif /* wukong_mysql_utils_h */

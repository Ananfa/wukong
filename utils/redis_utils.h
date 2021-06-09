#ifndef wukong_redis_utils_h
#define wukong_redis_utils_h

#include <string>
#include <list>
#include <stdint.h>
#include <hiredis.h>
#include "define.h"

namespace wukong {
    
    class RedisUtils {
    public:
        static uint64_t CreateUserID(redisContext *redis);
        static uint64_t CreateRoleID(redisContext *redis);
        static bool BindRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, UserId userId, ServerId serverId, uint32_t maxRoleNum);
        static bool LoadProfile(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas);
        static bool SaveProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas);
        static bool UpdateProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas);
        static bool LoadRole(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas);
        static bool SaveRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas);
        static bool UpdateRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas);
    };

}

#endif /* wukong_redis_utils_h */

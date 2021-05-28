#ifndef wukong_redis_utils_h
#define wukong_redis_utils_h

#include <string>
#include <stdint.h>
#include <hiredis.h>

namespace wukong {
    
    class RedisUtils {
    public:
        static uint64_t CreateUserID(redisContext *redis);
        static uint64_t CreateRoleID(redisContext *redis);
        static bool LoadProfile(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas);
        static bool SaveProfile(redisContext *redis, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &pDatas);
    };

}

#endif /* wukong_redis_utils_h */

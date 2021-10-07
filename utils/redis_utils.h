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
        static RedisAccessResult BindRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, UserId userId, ServerId serverId, uint32_t maxRoleNum);
        static RedisAccessResult LoadProfile(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult SaveProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult UpdateProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult LoadRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas, bool clearTTL);
        static RedisAccessResult SaveRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult UpdateRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas);

        static RedisAccessResult CheckPassport(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId &roleId);
        static RedisAccessResult SetSession(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId roleId);
        static RedisAccessResult GetGameObjectAddress(redisContext *redis, RoleId roleId, GameServerType &stype, ServerId &sid, uint32_t &ltoken);
    };

}

#endif /* wukong_redis_utils_h */

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

        static RedisAccessResult LoadSha1(redisContext *redis, const std::string &script, std::string &sha1);

        static RedisAccessResult LoginLock(redisContext *redis, const std::string &account);
        static RedisAccessResult GetUserID(redisContext *redis, const std::string &account, UserId &userId);
        static RedisAccessResult SetUserID(redisContext *redis, const std::string &account, UserId userId);
        static RedisAccessResult GetUserRoleIdList(redisContext *redis, UserId userId, std::vector<RoleId> &roleIds);
        static RedisAccessResult GetRoleCount(redisContext *redis, UserId userId, ServerId serverId, uint32_t &count); // 这里的serverId是逻辑区服号
        static RedisAccessResult CheckRole(redisContext *redis, UserId userId, ServerId serverId, RoleId roleId, bool &valid);
        static RedisAccessResult SetLoginToken(redisContext *redis, UserId userId, const std::string &token);
        static RedisAccessResult GetLoginToken(redisContext *redis, UserId userId, std::string &token);
        static RedisAccessResult RemoveLoginToken(redisContext *redis, UserId userId);
        static RedisAccessResult CreateRoleLock(redisContext *redis, UserId userId);
        static RedisAccessResult GetServerGroupsData(redisContext *redis, std::string &data);

        static RedisAccessResult BindRole(redisContext *redis, RoleId roleId, UserId userId, ServerId serverId, uint32_t maxRoleNum);
        static RedisAccessResult LoadProfile(redisContext *redis, RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult SaveProfile(redisContext *redis, RoleId roleId, UserId userId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult UpdateProfile(redisContext *redis, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult LoadRole(redisContext *redis, RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas, bool clearTTL);
        static RedisAccessResult SaveRole(redisContext *redis, RoleId roleId, UserId userId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult UpdateRole(redisContext *redis, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas);
        static RedisAccessResult SetRoleTTL(redisContext *redis, RoleId roleId);

        static RedisAccessResult SetPassport(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId roleId);
        static RedisAccessResult CheckPassport(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId &roleId);
        static RedisAccessResult SetSession(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId roleId);
        static RedisAccessResult GetSession(redisContext *redis, UserId userId, ServerId &gateId, std::string &gToken, RoleId &roleId);
        static RedisAccessResult SetSessionTTL(redisContext *redis, UserId userId, const std::string &gToken);
        static RedisAccessResult RemoveSession(redisContext *redis, UserId userId, const std::string &gToken);

        static RedisAccessResult GetGameObjectAddress(redisContext *redis, RoleId roleId, GameServerType &stype, ServerId &sid, std::string &ltoken);
        static RedisAccessResult SetGameObjectAddress(redisContext *redis, RoleId roleId, GameServerType stype, ServerId sid, const std::string &lToken);
        static RedisAccessResult RemoveGameObjectAddress(redisContext *redis, RoleId roleId, const std::string &lToken);
        static RedisAccessResult SetGameObjectAddressTTL(redisContext *redis, RoleId roleId, const std::string &lToken);

        static RedisAccessResult GetRecordAddress(redisContext *redis, RoleId roleId, ServerId &recordId);
        static RedisAccessResult SetRecordAddress(redisContext *redis, RoleId roleId, ServerId recordId, const std::string &rToken);
        static RedisAccessResult RemoveRecordAddress(redisContext *redis, RoleId roleId, const std::string &rToken);
        static RedisAccessResult SetRecordAddressTTL(redisContext *redis, RoleId roleId, const std::string &rToken);

        static RedisAccessResult SaveLock(redisContext *redis, uint32_t wheelPos);
        static RedisAccessResult GetSaveList(redisContext *redis, uint32_t wheelPos, std::vector<RoleId> &roleIds);
        static RedisAccessResult AddSaveRoleId(redisContext *redis, uint32_t wheelPos, RoleId roleId);
        static RedisAccessResult RemoveSaveRoleId(redisContext *redis, uint32_t wheelPos, RoleId roleId);

        static RedisAccessResult SetSceneAddress(redisContext *redis, const std::string &sceneId, const std::string &sToken, ServerId sceneServerId);
        static RedisAccessResult RemoveSceneAddress(redisContext *redis, const std::string &sceneId, const std::string &sToken);
        static RedisAccessResult SetSceneAddressTTL(redisContext *redis, const std::string &sceneId, const std::string &sToken);
    };

}

#endif /* wukong_redis_utils_h */

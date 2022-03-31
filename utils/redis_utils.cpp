#include <string.h>
#include <vector>
#include <assert.h>
#include "corpc_utils.h"
#include "redis_utils.h"
#include "redis_pool.h"
#include "const.h"

using namespace wukong;

uint64_t RedisUtils::CreateUserID(redisContext *redis) {
    int r = rand() % 100;
    redisReply *reply = (redisReply *)redisCommand(redis, "INCR UIDGEN:USER:%d", r);
    if (!reply) {
        return 0;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return 0;
    }

    uint64_t ret = reply->integer * 100 + r;
    freeReplyObject(reply);
    return ret;
}

uint64_t RedisUtils::CreateRoleID(redisContext *redis) {
    int r = rand() % 100;
    redisReply *reply = (redisReply *)redisCommand(redis, "INCR UIDGEN:ROLE:%d", r);
    if (!reply) {
        return 0;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return 0;
    }

    uint64_t ret = reply->integer * 100 + r;
    freeReplyObject(reply);
    return ret;
}

RedisAccessResult RedisUtils::LoadSha1(redisContext *redis, const std::string &script, std::string &sha1) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SCRIPT LOAD %s", script.c_str());
    if (!reply) {
        return REDIS_DB_ERROR;
    }
    
    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    sha1 = reply->str;
    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::LoginLock(redisContext *redis, const std::string &account) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SET LoginLock:{%s} 1 NX EX %d", account.c_str(), LOGIN_LOCK_TIME);
    if (!reply) {
        return REDIS_DB_ERROR;
    } 

    if (reply->str == nullptr || strcmp(reply->str, "OK")) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetUserID(redisContext *redis, const std::string &account, UserId &userId) {
    redisReply *reply = (redisReply *)redisCommand(redis, "GET O2U:{%s}", account.c_str());
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type == REDIS_REPLY_STRING) {
        userId = atoi(reply->str);
        freeReplyObject(reply);
        return REDIS_SUCCESS;
    }

    freeReplyObject(reply);
    return REDIS_FAIL;
}

RedisAccessResult RedisUtils::SetUserID(redisContext *redis, const std::string &account, UserId userId) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SET O2U:{%s} %llu NX", account.c_str(), userId);
    if (!reply) {
        return REDIS_DB_ERROR;
    } 

    if (reply->str == nullptr || strcmp(reply->str, "OK")) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetUserRoleIdList(redisContext *redis, UserId userId, std::vector<RoleId> &roleIds) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SMEMBERS RoleIds:{%llu}", userId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    if (reply->elements > 0) {
        roleIds.reserve(reply->elements);
        for (int i = 0; i < reply->elements; i++) {
            RoleId roleId = atoi(reply->element[i]->str);
            roleIds.push_back(roleId);
        }
    }
    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetRoleCount(redisContext *redis, UserId userId, ServerId serverId, uint32_t &count) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SCARD RoleIds:%d:{%llu}", serverId, userId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    count = reply->integer;
    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::CheckRole(redisContext *redis, UserId userId, ServerId serverId, RoleId roleId, bool &valid) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SISMEMBER RoleIds:%d:{%llu} %llu", serverId, userId, roleId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    valid = reply->integer == 1;
    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetLoginToken(redisContext *redis, UserId userId, const std::string &token) {
    // 以Token:[userId]为key，value为token，记录到redis数据库（有效期1小时？创角时延长有效期，玩家进入游戏时清除token）
    redisReply *reply = (redisReply *)redisCommand(redis, "SET Token:%llu %s EX 3600", userId, token.c_str());
    if (!reply) {
        return REDIS_DB_ERROR;
    } 

    if (reply->str == nullptr || strcmp(reply->str, "OK")) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetLoginToken(redisContext *redis, UserId userId, std::string &token) {
    redisReply *reply = (redisReply *)redisCommand(redis, "GET Token:%llu", userId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    token = reply->str;
    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::RemoveLoginToken(redisContext *redis, UserId userId) {
    // 删除登录临时token
    redisReply *reply = (redisReply *)redisCommand(redis,"DEL Token:%llu", userId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::CreateRoleLock(redisContext *redis, UserId userId) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SET CreateRole:{%llu} 1 NX EX 60", userId);
    if (!reply) {
        return REDIS_DB_ERROR;
    } 

    if (reply->str == nullptr || strcmp(reply->str, "OK")) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetServerGroupsData(redisContext *redis, std::string &data) {
    redisReply *reply = (redisReply *)redisCommand(redis, "GET ServerGroups");
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_STRING) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    data = reply->str;
    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::BindRole(redisContext *redis, RoleId roleId, UserId userId, ServerId serverId, uint32_t maxRoleNum) {
    const char *cmdSha1 = g_RedisPoolManager.getCorePersist()->getSha1(BIND_ROLE_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 2 RoleIds:%d:{%llu} RoleIds:{%llu} %llu %d", BIND_ROLE_CMD, serverId, userId, userId, roleId, maxRoleNum);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 2 RoleIds:%d:{%llu} RoleIds:{%llu} %llu %d", cmdSha1, serverId, userId, userId, roleId, maxRoleNum);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        // 设置失败
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);

    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::LoadProfile(redisContext *redis, RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HGETALL Profile:{%llu}", roleId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->elements == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    for (int i = 0; i < reply->elements; i += 2) {
        if (strcmp(reply->element[i]->str, "userId") == 0) {
            userId = atoi(reply->element[i+1]->str);
        } else if (strcmp(reply->element[i]->str, "serverId") == 0) {
            serverId = atoi(reply->element[i+1]->str);
        } else {
            datas.push_back(std::make_pair(reply->element[i]->str, reply->element[i+1]->str));
        }
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SaveProfile(redisContext *redis, RoleId roleId, UserId userId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SAVE_PROFILE_NAME);
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 9;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (!cmdSha1) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(SAVE_PROFILE_CMD);
        argvlen.push_back(strlen(SAVE_PROFILE_CMD));
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1);
        argvlen.push_back(strlen(cmdSha1));
    }
    
    argv.push_back("1");
    argvlen.push_back(1);

    char tmpStr[50];
    sprintf(tmpStr,"Profile:{%llu}", roleId);
    argv.push_back(tmpStr);
    argvlen.push_back(strlen(tmpStr));

    argv.push_back("86400");    // 1天超时时间
    argvlen.push_back(5);

    argv.push_back("userId");
    argvlen.push_back(6);
    std::string userIdStr = std::to_string(userId);
    argv.push_back(userIdStr.c_str());
    argvlen.push_back(userIdStr.length());

    argv.push_back("serverId");
    argvlen.push_back(8);
    std::string serverIdStr = std::to_string(serverId);
    argv.push_back(serverIdStr.c_str());
    argvlen.push_back(serverIdStr.length());

    for (auto &data : datas) {
        argv.push_back(data.first.c_str());
        argvlen.push_back(data.first.length());
        argv.push_back(data.second.c_str());
        argvlen.push_back(data.second.length());
    }
    
    redisReply *reply = (redisReply *)redisCommandArgv(redis, argv.size(), &(argv[0]), &(argvlen[0]));
    
    if (!reply) {
        return REDIS_DB_ERROR;
    } else if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);

    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::UpdateProfile(redisContext *redis, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas) {
    // 将轮廓数据存到cache中，若cache中没有找到则不需要更新轮廓数据
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(UPDATE_PROFILE_NAME);
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 5;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (!cmdSha1) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(UPDATE_PROFILE_CMD);
        argvlen.push_back(strlen(UPDATE_PROFILE_CMD));
        argv.push_back("1");
        argvlen.push_back(1);
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1);
        argvlen.push_back(strlen(cmdSha1));
        argv.push_back("1");
        argvlen.push_back(1);
    }

    char tmpStr[50];
    sprintf(tmpStr,"Profile:{%llu}", roleId);
    argv.push_back(tmpStr);
    argvlen.push_back(strlen(tmpStr));

    argv.push_back("86400");    // 1天超时时间
    argvlen.push_back(5);

    for (auto &data : datas)
    {
        argv.push_back(data.first.c_str());
        argvlen.push_back(data.first.length());
        argv.push_back(data.second.c_str());
        argvlen.push_back(data.second.length());
    }
    
    redisReply *reply = (redisReply *)redisCommandArgv(redis, argv.size(), &(argv[0]), &(argvlen[0]));
    
    if (!reply) {
        return REDIS_DB_ERROR;
    } else if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::LoadRole(redisContext *redis, RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas, bool clearTTL) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(LOAD_ROLE_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Role:{%llu} %d", LOAD_ROLE_CMD, roleId, clearTTL ? 1 : 0);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Role:{%llu} %d", cmdSha1, roleId, clearTTL ? 1 : 0);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->elements == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    for (int i = 0; i < reply->elements; i += 2) {
        if (strcmp(reply->element[i]->str, "userId") == 0) {
            userId = atoi(reply->element[i+1]->str);
        } else if (strcmp(reply->element[i]->str, "serverId") == 0) {
            serverId = atoi(reply->element[i+1]->str);
        } else {
            datas.push_back(std::make_pair(reply->element[i]->str, reply->element[i+1]->str));
        }
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SaveRole(redisContext *redis, RoleId roleId, UserId userId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SAVE_ROLE_NAME);
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 8;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (!cmdSha1) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(SAVE_ROLE_CMD);
        argvlen.push_back(strlen(SAVE_ROLE_CMD));
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1);
        argvlen.push_back(strlen(cmdSha1));
    }
    
    argv.push_back("1");
    argvlen.push_back(1);

    char tmpStr[50];
    sprintf(tmpStr,"Role:{%llu}", roleId);
    argv.push_back(tmpStr);
    argvlen.push_back(strlen(tmpStr));

    argv.push_back("userId");
    argvlen.push_back(6);
    std::string userIdStr = std::to_string(userId);
    argv.push_back(userIdStr.c_str());
    argvlen.push_back(userIdStr.length());

    argv.push_back("serverId");
    argvlen.push_back(8);
    std::string serverIdStr = std::to_string(serverId);
    argv.push_back(serverIdStr.c_str());
    argvlen.push_back(serverIdStr.length());

    for (auto &data : datas) {
        argv.push_back(data.first.c_str());
        argvlen.push_back(data.first.length());
        argv.push_back(data.second.c_str());
        argvlen.push_back(data.second.length());
    }
    
    redisReply *reply = (redisReply *)redisCommandArgv(redis, argv.size(), &(argv[0]), &(argvlen[0]));
    
    if (!reply) {
        return REDIS_DB_ERROR;
    } else if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::UpdateRole(redisContext *redis, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas) {
    // 将角色数据存到cache中
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(UPDATE_ROLE_NAME);
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 4;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (!cmdSha1) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(UPDATE_ROLE_CMD);
        argvlen.push_back(strlen(UPDATE_ROLE_CMD));
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1);
        argvlen.push_back(strlen(cmdSha1));
    }
    
    argv.push_back("1");
    argvlen.push_back(1);

    char tmpStr[50];
    sprintf(tmpStr,"Role:{%llu}", roleId);
    argv.push_back(tmpStr);
    argvlen.push_back(strlen(tmpStr));

    for (auto &data : datas) {
        argv.push_back(data.first.c_str());
        argvlen.push_back(data.first.length());
        argv.push_back(data.second.c_str());
        argvlen.push_back(data.second.length());
    }
    
    redisReply *reply = (redisReply *)redisCommandArgv(redis, argv.size(), &(argv[0]), &(argvlen[0]));
    
    if (!reply) {
        return REDIS_DB_ERROR;
    } else if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetRoleTTL(redisContext *redis, RoleId roleId) {
    // 设置cache数据超时时间
    redisReply *reply = (redisReply *)redisCommand(redis, "EXPIRE Role:{%llu} %d", roleId, RECORD_EXPIRE);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetPassport(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId roleId) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_PASSPORT_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Passport:%llu %s %d %llu %d", SET_PASSPORT_CMD, userId, gToken.c_str(), gateId, roleId, PASSPORT_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Passport:%llu %s %d %llu %d", cmdSha1, userId, gToken.c_str(), gateId, roleId, PASSPORT_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::CheckPassport(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId &roleId) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(CHECK_PASSPORT_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Passport:%llu %d %s", CHECK_PASSPORT_CMD, userId, gateId, gToken.c_str());
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Passport:%llu %d %s", cmdSha1, userId, gateId, gToken.c_str());
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    roleId = reply->integer;
    if (reply->type != REDIS_REPLY_INTEGER || roleId == 0) {
        freeReplyObject(reply);
        return REDIS_REPLY_INVALID;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetSession(redisContext *redis, UserId userId, ServerId gateId, const std::string &gToken, RoleId roleId) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_SESSION_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Session:%llu %s %d %llu %d", SET_SESSION_CMD, userId, gToken.c_str(), gateId, roleId, TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Session:%llu %s %d %llu %d", cmdSha1, userId, gToken.c_str(), gateId, roleId, TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        // 设置失败
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetSession(redisContext *redis, UserId userId, ServerId &gateId, std::string &gToken, RoleId &roleId) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HGETALL Session:%llu", userId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_ARRAY || reply->elements % 2 != 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    if (reply->elements > 0) {
        for (int i = 0; i < reply->elements; i += 2) {
            if (strcmp(reply->element[i]->str, "gateId") == 0) {
                gateId = atoi(reply->element[i+1]->str);
            } else if (strcmp(reply->element[i]->str, "gToken") == 0) {
                gToken = reply->element[i+1]->str;
            } else if (strcmp(reply->element[i]->str, "roleId") == 0) {
                roleId = atoi(reply->element[i+1]->str);
            }
        }
    } else {
        gateId = 0;
        gToken = "";
        roleId = 0;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetSessionTTL(redisContext *redis, UserId userId, const std::string &gToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_SESSION_EXPIRE_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Session:%llu %s %d", SET_SESSION_EXPIRE_CMD, userId, gToken.c_str(), TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Session:%llu %s %d", cmdSha1, userId, gToken.c_str(), TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::RemoveSession(redisContext *redis, UserId userId, const std::string &gToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(REMOVE_SESSION_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Session:%llu %s", REMOVE_SESSION_CMD, userId, gToken.c_str());
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Session:%llu %s", cmdSha1, userId, gToken.c_str());
    }

    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetGameObjectAddress(redisContext *redis, RoleId roleId, GameServerType &stype, ServerId &sid, std::string &lToken) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HMGET Location:%llu lToken stype sid", roleId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_ARRAY || reply->elements != 3) {
        freeReplyObject(reply);
        return REDIS_REPLY_INVALID;
    }

    if (reply->element[0]->type == REDIS_REPLY_STRING) {
        assert(reply->element[1]->type == REDIS_REPLY_STRING);
        assert(reply->element[2]->type == REDIS_REPLY_STRING);
        // 这里不再向游戏对象所在服发RPC（极端情况是游戏对象刚巧销毁导致网关对象指向了不存在的游戏对象的游戏服务器，网关对象一段时间没有收到游戏对象心跳后销毁）
        lToken = reply->element[0]->str;
        stype = atoi(reply->element[1]->str);
        sid = atoi(reply->element[2]->str);

        freeReplyObject(reply);
        return REDIS_SUCCESS;
    }

    assert(reply->element[0]->type == REDIS_REPLY_NIL);
    assert(reply->element[1]->type == REDIS_REPLY_NIL);
    assert(reply->element[2]->type == REDIS_REPLY_NIL);
    freeReplyObject(reply);
    return REDIS_FAIL;
}

RedisAccessResult RedisUtils::SetGameObjectAddress(redisContext *redis, RoleId roleId, GameServerType stype, ServerId sid, const std::string &lToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_LOCATION_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Location:%llu %s %d %d %d", SET_LOCATION_CMD, roleId, lToken.c_str(), stype, sid, TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Location:%llu %s %d %d %d", cmdSha1, roleId, lToken.c_str(), stype, sid, TOKEN_TIMEOUT);
    }

    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::RemoveGameObjectAddress(redisContext *redis, RoleId roleId, const std::string &lToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(REMOVE_LOCATION_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Location:%llu %s", REMOVE_LOCATION_CMD, roleId, lToken.c_str());
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Location:%llu %s", cmdSha1, roleId, lToken.c_str());
    }

    if (!reply) {
        return REDIS_DB_ERROR;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetGameObjectAddressTTL(redisContext *redis, RoleId roleId, const std::string &lToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_LOCATION_EXPIRE_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Location:%llu %s %d", SET_LOCATION_EXPIRE_CMD, roleId, lToken.c_str(), TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Location:%llu %s %d", cmdSha1, roleId, lToken.c_str(), TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetRecordAddress(redisContext *redis, RoleId roleId, ServerId &recordId) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HGET Record:%llu loc", roleId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type == REDIS_REPLY_NIL) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    recordId = std::stoi(reply->str);
    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetRecordAddress(redisContext *redis, RoleId roleId, ServerId recordId, const std::string &rToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_RECORD_NAME);
    redisReply *reply;
    // 尝试设置record
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Record:%llu %s %d %d", SET_RECORD_CMD, roleId, rToken.c_str(), recordId, TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Record:%llu %s %d %d", cmdSha1, roleId, rToken.c_str(), recordId, TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::RemoveRecordAddress(redisContext *redis, RoleId roleId, const std::string &rToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(REMOVE_RECORD_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Record:%llu %s", REMOVE_RECORD_CMD, roleId, rToken.c_str());
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Record:%llu %s", cmdSha1, roleId, rToken.c_str());
    }

    if (!reply) {
        return REDIS_DB_ERROR;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetRecordAddressTTL(redisContext *redis, RoleId roleId, const std::string &rToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_RECORD_EXPIRE_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Record:%llu %s %d", SET_RECORD_EXPIRE_CMD, roleId, rToken.c_str(), TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Record:%llu %s %d", cmdSha1, roleId, rToken.c_str(), TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SaveLock(redisContext *redis, uint32_t wheelPos) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SET SaveLock:%d 1 NX EX 60", wheelPos);
    if (!reply) {
        return REDIS_DB_ERROR;
    } 

    if (strcmp(reply->str, "OK")) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::GetSaveList(redisContext *redis, uint32_t wheelPos, std::vector<RoleId> &roleIds) {
    // 读取整个SET
    redisReply *reply = (redisReply *)redisCommand(redis, "SMEMBERS Save:%d", wheelPos);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->type != REDIS_REPLY_ARRAY) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    roleIds.clear();
    if (reply->elements > 0) {
        roleIds.reserve(reply->elements);
        for (int i = 0; i < reply->elements; i++) {
            RoleId roleId = atoi(reply->element[i]->str);
            if (roleId == 0) {
                ERROR_LOG("RedisUtils::GetSaveList -- invalid roleid\n");
                continue;
            }

            roleIds.push_back(roleId);
        }
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::AddSaveRoleId(redisContext *redis, uint32_t wheelPos, RoleId roleId) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SADD Save:%d %llu", wheelPos, roleId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::RemoveSaveRoleId(redisContext *redis, uint32_t wheelPos, RoleId roleId) {
    redisReply *reply = (redisReply *)redisCommand(redis, "SREM Save:%d %llu", wheelPos, roleId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetSceneAddress(redisContext *redis, const std::string &sceneId, const std::string &sToken, ServerId sceneServerId) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_SCENE_LOCATION_NAME);
    redisReply *reply;
    // 尝试设置scene location
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Scene:%s %s %d %d", SET_SCENE_LOCATION_CMD, sceneId.c_str(), sToken.c_str(), sceneServerId, TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Scene:%s %s %d %d", cmdSha1, sceneId.c_str(), sToken.c_str(), sceneServerId, TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::RemoveSceneAddress(redisContext *redis, const std::string &sceneId, const std::string &sToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(REMOVE_SCENE_LOCATION_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Scene:%s %s", REMOVE_SCENE_LOCATION_CMD, sceneId.c_str(), sToken.c_str());
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Scene:%s %s", cmdSha1, sceneId.c_str(), sToken.c_str());
    }

    if (!reply) {
        return REDIS_DB_ERROR;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SetSceneAddressTTL(redisContext *redis, const std::string &sceneId, const std::string &sToken) {
    const char *cmdSha1 = g_RedisPoolManager.getCoreCache()->getSha1(SET_SCENE_LOCATION_EXPIRE_NAME);
    redisReply *reply;
    if (!cmdSha1) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Scene:%s %s %d", SET_SCENE_LOCATION_EXPIRE_CMD, sceneId.c_str(), sToken.c_str(), TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Scene:%s %s %d", cmdSha1, sceneId.c_str(), sToken.c_str(), TOKEN_TIMEOUT);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->integer == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

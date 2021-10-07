#include <string.h>
#include <vector>
#include "corpc_utils.h"
#include "redis_utils.h"
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

RedisAccessResult RedisUtils::BindRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, UserId userId, ServerId serverId, uint32_t maxRoleNum) {
    redisReply *reply;
    if (cmdSha1.empty()) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 2 RoleIds:%d:{%d} RoleIds:{%d} %d %d", BIND_ROLE_CMD, serverId, userId, userId, roleId, maxRoleNum);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 2 RoleIds:%d:{%d} RoleIds:{%d} %d %d", cmdSha1.c_str(), serverId, userId, userId, roleId, maxRoleNum);
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

RedisAccessResult RedisUtils::LoadProfile(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HGETALL Profile:{%d}", roleId);
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->elements == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    for (int i = 0; i < reply->elements; i += 2) {
        if (strcmp(reply->element[i]->str, "serverId") == 0) {
            serverId = atoi(reply->element[i+1]->str);
        } else {
            datas.push_back(std::make_pair(reply->element[i]->str, reply->element[i+1]->str));
        }
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SaveProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas) {
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 7;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (cmdSha1.empty()) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(SAVE_PROFILE_CMD);
        argvlen.push_back(strlen(SAVE_PROFILE_CMD));
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1.c_str());
        argvlen.push_back(cmdSha1.length());
    }
    
    argv.push_back("1");
    argvlen.push_back(1);

    char tmpStr[50];
    sprintf(tmpStr,"Profile:{%d}", roleId);
    argv.push_back(tmpStr);
    argvlen.push_back(strlen(tmpStr));

    argv.push_back("86400");    // 1天超时时间
    argvlen.push_back(5);

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

RedisAccessResult RedisUtils::UpdateProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas) {
    // 将轮廓数据存到cache中，若cache中没有找到则不需要更新轮廓数据
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 5;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (cmdSha1.empty()) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(UPDATE_PROFILE_CMD);
        argvlen.push_back(strlen(UPDATE_PROFILE_CMD));
        argv.push_back("1");
        argvlen.push_back(1);
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1.c_str());
        argvlen.push_back(cmdSha1.length());
        argv.push_back("1");
        argvlen.push_back(1);
    }

    char tmpStr[50];
    sprintf(tmpStr,"Profile:{%d}", roleId);
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

RedisAccessResult RedisUtils::LoadRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas, bool clearTTL) {
    redisReply *reply;
    if (cmdSha1.empty()) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Role:{%d} %d", LOAD_ROLE_CMD, roleId, clearTTL ? 1 : 0);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Role:{%d} %d", cmdSha1.c_str(), roleId, clearTTL ? 1 : 0);
    }
    
    if (!reply) {
        return REDIS_DB_ERROR;
    }

    if (reply->elements == 0) {
        freeReplyObject(reply);
        return REDIS_FAIL;
    }

    for (int i = 0; i < reply->elements; i += 2) {
        if (strcmp(reply->element[i]->str, "serverId") == 0) {
            serverId = atoi(reply->element[i+1]->str);
        } else {
            datas.push_back(std::make_pair(reply->element[i]->str, reply->element[i+1]->str));
        }
    }

    freeReplyObject(reply);
    return REDIS_SUCCESS;
}

RedisAccessResult RedisUtils::SaveRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas) {
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 6;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (cmdSha1.empty()) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(SAVE_ROLE_CMD);
        argvlen.push_back(strlen(SAVE_ROLE_CMD));
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1.c_str());
        argvlen.push_back(cmdSha1.length());
    }
    
    argv.push_back("1");
    argvlen.push_back(1);

    char tmpStr[50];
    sprintf(tmpStr,"Role:{%d}", roleId);
    argv.push_back(tmpStr);
    argvlen.push_back(strlen(tmpStr));

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

RedisAccessResult RedisUtils::UpdateRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas) {
    // 将角色数据存到cache中
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 4;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    if (cmdSha1.empty()) {
        argv.push_back("EVAL");
        argvlen.push_back(4);
        argv.push_back(UPDATE_ROLE_CMD);
        argvlen.push_back(strlen(UPDATE_ROLE_CMD));
    } else {
        argv.push_back("EVALSHA");
        argvlen.push_back(7);
        argv.push_back(cmdSha1.c_str());
        argvlen.push_back(cmdSha1.length());
    }
    
    argv.push_back("1");
    argvlen.push_back(1);

    char tmpStr[50];
    sprintf(tmpStr,"Role:{%d}", roleId);
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

RedisAccessResult RedisUtils::CheckPassport(redisContext *redis, const std::string &cmdSha1, UserId userId, ServerId gateId, const std::string &gToken, RoleId &roleId) {
    redisReply *reply;
    if (cmdSha1.empty()) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Passport:%d %d %s", CHECK_PASSPORT_CMD, userId, gateId, gToken.c_str());
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Passport:%d %d %s", cmdSha1.c_str(), userId, gateId, gToken.c_str());
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

RedisAccessResult RedisUtils::SetSession(redisContext *redis, const std::string &cmdSha1, UserId userId, ServerId gateId, const std::string &gToken, RoleId roleId) {
    redisReply *reply;
    if (cmdSha1.empty()) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 1 Session:%d %s %d %d %d", SET_SESSION_CMD, userId, gToken.c_str(), gateId, roleId, TOKEN_TIMEOUT);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 1 Session:%d %s %d %d %d", cmdSha1.c_str(), userId, gToken.c_str(), gateId, roleId, TOKEN_TIMEOUT);
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

RedisAccessResult RedisUtils::GetGameObjectAddress(redisContext *redis, RoleId roleId, GameServerType &stype, ServerId &sid, uint32_t &ltoken) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HMGET Location:%d lToken stype sid", roleId);
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
        lToken = atoi(reply->element[0]->str);
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
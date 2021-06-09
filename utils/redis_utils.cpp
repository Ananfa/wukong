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

bool RedisUtils::BindRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, UserId userId, ServerId serverId, uint32_t maxRoleNum) {
    redisReply *reply;
    if (cmdSha1.empty()) {
        reply = (redisReply *)redisCommand(redis, "EVAL %s 2 RoleIds:%d:{%d} %d %d", BIND_ROLE_CMD, serverId, userId, roleId, maxRoleNum);
    } else {
        reply = (redisReply *)redisCommand(redis, "EVALSHA %s 2 RoleIds:%d:{%d} %d %d", cmdSha1.c_str(), serverId, userId, roleId, maxRoleNum);
    }
    
    if (!reply) {
        ERROR_LOG("RedisUtils::BindRoleId -- bind roleId:[%d] userId:[%d] serverId:[%d] failed\n", roleId, userId, serverId);
        return false;
    }

    if (reply->type != REDIS_REPLY_INTEGER) {
        ERROR_LOG("RedisUtils::BindRoleId -- bind roleId:[%d] userId:[%d] serverId:[%d] failed for return type invalid\n", roleId, userId, serverId);
        freeReplyObject(reply);
        return false;
    }

    if (reply->integer == 0) {
        ERROR_LOG("RedisUtils::BindRoleId -- bind roleId:[%d] userId:[%d] serverId:[%d] failed for reach the limit of the number of roles\n", roleId, userId, serverId);
        // 设置失败
        freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);

    return true;
}

bool RedisUtils::LoadProfile(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HGETALL Profile:{%d}", roleId);
    if (!reply) {
        return false;
    }

    if (reply->type != REDIS_REPLY_ARRAY || reply->elements % 2 != 0) {
        freeReplyObject(reply);
        return false;
    }

    if (reply->elements > 0) {
        for (int i = 0; i < reply->elements; i += 2) {
            if (strcmp(reply->element[i]->str, "serverId") == 0) {
                serverId = atoi(reply->element[i+1]->str);
            } else {
                datas.push_back(std::make_pair(reply->element[i]->str, reply->element[i+1]->str));
            }
        }

        freeReplyObject(reply);
        return true;
    }

    freeReplyObject(reply);
    return true;
}

bool RedisUtils::SaveProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas) {
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
        ERROR_LOG("RedisUtils::SaveProfile -- role %d cache profile failed for db error\n", roleId);
        return false;
    } else if (reply->integer == 0) {
        ERROR_LOG("RedisUtils::SaveProfile -- role %d cache profile failed: %s\n", roleId, reply->str);
        freeReplyObject(reply);

        return false;
    }

    freeReplyObject(reply);

    return true;
}

bool RedisUtils::UpdateProfile(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas) {
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
        ERROR_LOG("RedisUtils::UpdateProfile -- role %d cache data failed for db error\n", roleId);
        return false;
    } else if (reply->integer == 0) {
        ERROR_LOG("RedisUtils::UpdateProfile -- role %d cache data failed: %s\n", roleId, reply->str);
        freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}

bool RedisUtils::LoadRole(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &datas) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HGETALL Role:{%d}", roleId);
    if (!reply) {
        return false;
    }

    if (reply->type != REDIS_REPLY_ARRAY || reply->elements % 2 != 0) {
        freeReplyObject(reply);
        return false;
    }

    if (reply->elements > 0) {
        for (int i = 0; i < reply->elements; i += 2) {
            if (strcmp(reply->element[i]->str, "serverId") == 0) {
                serverId = atoi(reply->element[i+1]->str);
            } else {
                datas.push_back(std::make_pair(reply->element[i]->str, reply->element[i+1]->str));
            }
        }

        freeReplyObject(reply);
        return true;
    }

    freeReplyObject(reply);
    return true;
}

bool RedisUtils::SaveRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &datas) {
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
        ERROR_LOG("RedisUtils::SaveProfile -- role %d save role failed for db error\n", roleId);
        return false;
    } else if (reply->integer == 0) {
        ERROR_LOG("RedisUtils::SaveProfile -- role %d save role failed: %s\n", roleId, reply->str);
        freeReplyObject(reply);

        return false;
    }

    freeReplyObject(reply);

    return true;
}

bool RedisUtils::UpdateRole(redisContext *redis, const std::string &cmdSha1, RoleId roleId, const std::list<std::pair<std::string, std::string>> &datas) {
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
        ERROR_LOG("RedisUtils::UpdateRole -- role %d update data failed for db error\n", roleId);
        return false;
    } else if (reply->integer == 0) {
        ERROR_LOG("RedisUtils::UpdateRole -- role %d update data failed: %s\n", roleId, reply->str);
        freeReplyObject(reply);
        return false;
    }

    freeReplyObject(reply);
    return true;
}
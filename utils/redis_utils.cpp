#include "redis_utils.h"

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

bool RedisUtils::LoadProfile(redisContext *redis, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas) {
    redisReply *reply = (redisReply *)redisCommand(redis, "HGETALL RoleInfo:{%d}", roleId);
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
                pDatas.push_back(std::make_pair(reply->element[i]->str, reply->element[i+1]->str));
            }
        }

        freeReplyObject(reply);
        return true;
    }

    freeReplyObject(reply);
    return true;
}

bool RedisUtils::SaveProfile(redisContext *redis, RoleId roleId, ServerId serverId, const std::list<std::pair<std::string, std::string>> &pDatas) {
    std::vector<const char *> argv;
    std::vector<size_t> argvlen;
    int argNum = datas.size() * 2 + 7;
    argv.reserve(argNum);
    argvlen.reserve(argNum);
    argv.push_back("EVAL");
    argvlen.push_back(4);
    argv.push_back(SET_PROFILE_CMD);
    argvlen.push_back(strlen(SET_PROFILE_CMD));
    argv.push_back("1");
    argvlen.push_back(1);

    char tmpStr[50];
    sprintf(tmpStr,"RoleInfo:{%d}", roleId);
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
    } else if (strcmp(reply->str, "OK")) {
        ERROR_LOG("RedisUtils::SaveProfile -- role %d cache profile failed: %s\n", roleId, reply->str);
        freeReplyObject(reply);

        return false;
    }

    freeReplyObject(reply);

    return true;
}
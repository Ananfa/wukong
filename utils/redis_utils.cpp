#include "redis_utils.h"

#include <vector>

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
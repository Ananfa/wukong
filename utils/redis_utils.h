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
    };

}

#endif /* wukong_redis_utils_h */

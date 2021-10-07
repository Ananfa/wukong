#ifndef define_h
#define define_h

#include <string>

// 服务器id
typedef uint16_t ServerId;

// 组id
typedef uint16_t GroupId;

// 玩家id
typedef uint64_t UserId;

// 角色id
typedef uint64_t RoleId;

// 服务器类型
typedef uint16_t GameServerType;

enum ServerStatus {
    SERVER_STATUS_NORMAL = 1,
    SERVER_STATUS_FULL = 2,
    SERVER_STATUS_CLOSED = 3
};

enum RedisAccessResult {
    REDIS_SUCCESS = 0,
    REDIS_FAIL = 1,
    REDIS_DB_ERROR = 2,
    REDIS_REPLY_INVALID = 3
};

namespace wukong {
    struct Address {
        std::string host; // ip或域名
        uint16_t port; // 端口
    };

    struct ServerWeightInfo {
        ServerId id;
        uint32_t weight;
    };
}

#endif /* define_h */

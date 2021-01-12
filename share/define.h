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

enum ServerStatus {
    SERVER_STATUS_NORMAL = 1,
    SERVER_STATUS_FULL = 2,
    SERVER_STATUS_CLOSED = 3
};

#endif /* define_h */

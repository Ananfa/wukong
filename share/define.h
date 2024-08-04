#ifndef wukong_define_h
#define wukong_define_h

#include "corpc_queue.h"
#include "inner_common.pb.h"
#include <string>

using namespace corpc;

// 服务器id
typedef uint16_t ServerId;

// 组id
typedef uint16_t GroupId;

// 玩家id
typedef uint64_t UserId;

// 角色id
typedef uint64_t RoleId;

// 服务器类型
//typedef uint16_t GameServerType;

typedef uint16_t ServerType;

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

enum SceneType {
    SCENE_TYPE_SINGLE_PLAYER = 1, // 个人副本（无人时销毁内存对象）
    SCENE_TYPE_MULTI_PLAYER = 2, // 多人副本（无人时销毁内存对象）
    SCENE_TYPE_TEAM = 3, // 队伍副本（只要有一个队员故障下线，场景及其他队员一并踢下线）
    SCENE_TYPE_GLOBAL = 4 // 世界（永久）场景（无人也不销毁内存对象）
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

    struct RedisInfo {
        std::string dbName;     // 库名
        std::string host;       // db服务器host
        std::string pwd;
        uint16_t port;          // db服务器port
        uint16_t dbIndex;       // db分库索引
        uint16_t maxConnect;    // 最大连接数
    };

    struct MysqlInfo {
        std::string host;
        uint16_t port;
        std::string user;
        std::string pwd;
        uint16_t maxConnect;    // 最大连接数
        std::string dbName;
    };

#ifdef USE_NO_LOCK_QUEUE
    typedef Co_MPSC_NoLockQueue<std::shared_ptr<wukong::pb::GlobalEventMessage>> GlobalEventQueue;
    typedef Co_MPSC_NoLockQueue<std::shared_ptr<GlobalEventQueue>> GlobalEventRegisterQueue;
#else
    typedef CoSyncQueue<std::shared_ptr<wukong::pb::GlobalEventMessage>> GlobalEventQueue;
    typedef CoSyncQueue<std::shared_ptr<GlobalEventQueue>> GlobalEventRegisterQueue;
#endif
}

#endif /* wukong_define_h */

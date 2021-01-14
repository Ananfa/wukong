#ifndef const_h
#define const_h

#include "define.h"
#include <string>

namespace wukong {
    // ZooKeeper core server node name
    const std::string ZK_ADMIN_SERVER              = "/admin_server";
    const std::string ZK_GATEWAY_SERVER            = "/gateway_server";
    const std::string ZK_LOBBY_SERVER              = "/lobby_server";
    const std::string ZK_LOGIN_SERVER              = "/login_server";
    const std::string ZK_MYSQL_DATA_SERVER         = "/mysql_data_server";
    const std::string ZK_PROFILE_SERVER            = "/profile_server";

    // TODO: other server
    const std::string ZK_BATTLE_SERVER             = "/battle_server";
    const std::string ZK_SCENE_SERVER              = "/scene_server";

    const GameServerType GAME_SERVER_TYPE_LOBBY    = 1;
    const GameServerType GAME_SERVER_TYPE_SCENE    = 2;

    const std::string ZK_DEFAULT_VALUE             = "1";
    const int ZK_TIMEOUT                           = 3000;

    // 客户端向服务器发的消息ID定义
    const uint16_t C2S_MESSAGE_ID_AUTH             = 1; // 客户端认证消息

    // 服务器向客户端发的消息ID定义
    const uint16_t S2C_MESSAGE_ID_BAN              = 1; // 消息被屏蔽消息

    const char SET_SESSION_CMD[] = "\
        local ret=redis.call('hsetnx',KEYS[1],'gToken',ARGV[1])\
        if ret==1 then\
          redis.call('hset',KEYS[1],'gateId',ARGV[2])\
          redis.call('hset',KEYS[1],'roleId',ARGV[3])\
          redis.call('expire',KEYS[1],60)\
          return 1\
        else\
          return 0\
        end";

    const char CHECK_SESSION_CMD[] = "\
        local gateId = redis.call('hget',KEYS[1],'gateId')\
        if not gateId then\
          return 0\
        elseif gateId ~= ARGV[1] then\
          return 0\
        end\
        local gToken = redis.call('hget',KEYS[1],'gToken')\
        if not gToken then\
          return 0\
        elseif gToken ~= ARGV[2] then\
          return 0\
        end\
        local roleId = redis.call('hget',KEYS[1],'roleId')\
        if not roleId then\
          return 0\
        end\
        local ret = redis.call('expire',KEYS[1],ARGV[3])\
        if ret == 0 then\
          return 0\
        end\
        return tonumber(roleId)";

    const char SET_SESSION_EXPIRE_CMD[] = "\
        local gToken = redis.call('hget',KEYS[1],'gToken')\
        if not gToken then\
          return 0\
        elseif gToken ~= ARGV[1] then\
          return 0\
        end\
        local ret = redis.call('expire',KEYS[1],ARGV[2])\
        return ret";



}

#endif /* const_h */

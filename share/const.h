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
    const std::string ZK_RECORD_SERVER             = "/record_server";
    const std::string ZK_PROFILE_SERVER            = "/profile_server";

    // TODO: other server
    const std::string ZK_BATTLE_SERVER             = "/battle_server";
    const std::string ZK_SCENE_SERVER              = "/scene_server";

    const GameServerType GAME_SERVER_TYPE_LOBBY    = 1;
    const GameServerType GAME_SERVER_TYPE_SCENE    = 2;

    const std::string ZK_DEFAULT_VALUE             = "1";
    const int ZK_TIMEOUT                           = 3000; // 单位毫秒

    const int TOKEN_TIMEOUT                        = 60; // 令牌超时时间，单位秒
    const int TOKEN_HEARTBEAT_PERIOD               = 20; // 令牌心跳周期，单位秒

    const int RECORD_TIMEOUT                       = 600; // 记录对象收不到游戏对象心跳的超时时间，单位秒
    const int RECORD_EXPIRE                        = 86400; // 记录对象销毁后cache数据超时时长，单位秒

    const int SYNC_PERIOD                          = 1; // 游戏服向记录服同步数据周期，单位秒
    const int CACHE_PERIOD                         = 10; // 记录服向redis缓存脏数据的周期，单位秒
    const int SAVE_PERIOD                          = 600; // 将redis数据落地到mysql的时间周期，单位秒

    // 客户端向服务器发的消息ID定义
    const uint16_t C2S_MESSAGE_ID_AUTH             = 1; // 客户端认证消息

    // 服务器向客户端发的消息ID定义
    const uint16_t S2C_MESSAGE_ID_BAN              = 1; // 消息被屏蔽消息
    const uint16_t S2C_MESSAGE_ID_ENTERGAME        = 2; // 进入游戏消息

    const char SET_SESSION_CMD[] = "\
        local ret=redis.call('hsetnx',KEYS[1],'gToken',ARGV[1])\
        if ret==1 then\
          redis.call('hmset',KEYS[1],'gateId',ARGV[2],'roleId',ARGV[3])\
          redis.call('expire',KEYS[1],ARGV[4])\
          return 1\
        else\
          return 0\
        end";

    const char REMOVE_SESSION_CMD[] = "\
        local gToken=redis.call('hget',KEYS[1],'gToken')\
        if not gToken then\
          return 0\
        elseif gToken ~= ARGV[1] then\
          return 0\
        end\
        redis.call('del',KEYS[1])\
        return 1";

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

    const char SET_LOCATION_CMD[] = "\
        local ret = redis.call('hsetnx',KEYS[1],'lToken',ARGV[1])\
        if ret==1 then\
          redis.call('hmset',KEYS[1],'stype',ARGV[2],'sid',ARGV[3])\
          redis.call('expire',KEYS[1],ARGV[4])\
          return 1\
        else\
          return 0\
        end";

    const char REMOVE_LOCATION_CMD[] = "\
        local lToken=redis.call('hget',KEYS[1],'lToken')\
        if not lToken then\
          return 0\
        elseif lToken ~= ARGV[1] then\
          return 0\
        end\
        redis.call('del',KEYS[1])\
        return 1";

    const char UPDATE_LOCATION_CMD[] = "\
        local lToken = redis.call('hget',KEYS[1],'lToken')\
        if not lToken then\
          return 0\
        elseif lToken ~= ARGV[1] then\
          return 0\
        end\
        redis.call('hmset',KEYS[1],'stype',ARGV[2],'sid',ARGV[3])\
        redis.call('expire',KEYS[1],ARGV[4])\
        return 1";

    const char SET_LOCATION_EXPIRE_CMD[] = "\
        local lToken = redis.call('hget',KEYS[1],'lToken')\
        if not lToken then\
          return 0\
        elseif lToken ~= ARGV[1] then\
          return 0\
        end\
        redis.call('expire',KEYS[1],ARGV[2])\
        return 1";

    const char SET_RECORD_CMD[] = "\
        local ret = redis.call('hsetnx',KEYS[1],'rToken',ARGV[1])\
        if ret==1 then\
          redis.call('expire',KEYS[1],ARGV[2])\
          return 1\
        else\
          return 0\
        end";

    const char REMOVE_RECORD_CMD[] = "\
        local rToken=redis.call('hget',KEYS[1],'rToken')\
        if not rToken then\
          return 0\
        elseif rToken ~= ARGV[1] then\
          return 0\
        end\
        redis.call('del',KEYS[1])\
        return 1";

    const char SET_RECORD_EXPIRE_CMD[] = "\
        local rToken = redis.call('hget',KEYS[1],'rToken')\
        if not rToken then\
          return 0\
        elseif rToken ~= ARGV[1] then\
          return 0\
        end\
        redis.call('expire',KEYS[1],ARGV[2])\
        return 1";

    const char BIND_ROLE_CMD[] = "\
        local ret = redis.call('scard',KEYS[1])\
        if ret<tonumber(ARGV[2]) then\
          redis.call('sadd',KEYS[1],ARGV[1])\
          redis.call('sadd',KEYS[2],ARGV[1])\
          return 1\
        else\
          return 0\
        end";

    const char SAVE_PROFILE_CMD[] = "\
        local ret = redis.call('exists',KEYS[1])\
        if ret==0 then\
          redis.call('hmset',KEYS[1],unpack(ARGV,2))\
          redis.call('expire',KEYS[1],ARGV[1])\
          return 1\
        else\
          return 0\
        end";

    const char UPDATE_PROFILE_CMD[] = "\
        local ret = redis.call('exists',KEYS[1])\
        if ret==1 then\
          redis.call('hmset',KEYS[1],unpack(ARGV,2))\
          redis.call('expire',KEYS[1],ARGV[1])\
          return 1\
        else\
          return 0\
        end";

    const char SAVE_ROLE_CMD[] = "\
        local ret = redis.call('exists',KEYS[1])\
        if ret==0 then\
          redis.call('hmset',KEYS[1],unpack(ARGV))\
          return 1\
        else\
          return 0\
        end";

    const char UPDATE_ROLE_CMD[] = "\
        local ret = redis.call('exists',KEYS[1])\
        if ret==1 then\
          redis.call('hmset',KEYS[1],unpack(ARGV))\
          return 1\
        else\
          return 0\
        end";

    const char LOAD_ROLE_CMD[] = "\
        local ret = redis.call('exists',KEYS[1])\
        if ret==0 then\
          return {}\
        end\
        if ARGV[1]==\"1\" then\
          redis.call('persist',KEYS[1])\
        end\
        return redis.call('hgetall',KEYS[1])";
}

#endif /* const_h */

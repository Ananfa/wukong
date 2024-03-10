// This file is generated. Don't edit it

#include "demo_utils.h"
#include "redis_utils.h"
#include "mysql_utils.h"
#include "redis_pool.h"
#include "mysql_pool.h"
#include "common.pb.h"

using namespace demo;
using namespace wukong;

bool DemoUtils::LoadProfile(RoleId roleId, UserId &userId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas) {
    serverId = 0;
    pDatas.clear();
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("connect to cache failed\n");
        return false;
    }

    switch (RedisUtils::LoadProfile(cache, roleId, userId, serverId, pDatas)) {
        case REDIS_SUCCESS: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            return true;
        }
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("load role profile failed for db error\n");
            return false;
        }
    }

    assert(pDatas.size() == 0);

    std::list<std::pair<std::string, std::string>> rDatas;

    if (RedisUtils::LoadRole(cache, roleId, userId, serverId, rDatas, false) == REDIS_DB_ERROR) {
        g_RedisPoolManager.getCoreCache()->put(cache, true);
        ERROR_LOG("load role data failed for db error\n");
        return false;
    }
    
    if (rDatas.size() > 0) {
        MakeProfile(rDatas, pDatas);

        switch (RedisUtils::SaveProfile(cache, roleId, userId, serverId, pDatas)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("save role profile failed for db error\n");
                return false;
            }
            case REDIS_FAIL: {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                ERROR_LOG("save role profile failed\n");
                return false;
            }
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
        return true;
    }
    
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    MYSQL *mysql = g_MysqlPoolManager.getCoreRecord()->take();
    if (!mysql) {
        ERROR_LOG("connect to mysql failed\n");
        return false;
    }

    std::string data;
    if (!MysqlUtils::LoadRole(mysql, roleId, userId, serverId, data)) {
        g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
        return false;
    }

    g_MysqlPoolManager.getCoreRecord()->put(mysql, false);

    if (data.empty()) {
        ERROR_LOG("no role data\n");
        return false;
    }

    wukong::pb::DataFragments fragments;
    if (!fragments.ParseFromString(data)) {
        ERROR_LOG("parse role:%d data failed\n", roleId);
        return false;
    }

    int fragNum = fragments.fragments_size();
    for (int i = 0; i < fragNum; i++) {
        auto &fragment = fragments.fragments(i);
        pDatas.push_back(std::make_pair(fragment.fragname(), fragment.fragdata()));
    }

    if (pDatas.size() == 0) {
        return false;
    }

    MakeProfile(rDatas, pDatas);

    cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("connect to cache failed\n");
        return false;
    }

    switch (RedisUtils::SaveProfile(cache, roleId, userId, serverId, pDatas)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("save role profile failed for db error\n");
            return false;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("save role profile failed\n");
            return false;
        }
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);
    return true;
}

void DemoUtils::MakeProfile(const std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &pDatas) {
    for (auto &data : datas) {
        if (data.first.compare("name") == 0|| 
            data.first.compare("lv") == 0) {
            pDatas.push_back(data);
        }
    }
}

// TODO：此文件通过工具生成

#include "demo_db_utils.h"
#include "demo_utils.h"
#include "redis_utils.h"
#include "mysql_utils.h"
#include "cache_pool.h"
#include "mysql_pool.h"
#include "common.pb.h"

using namespace demo;
using namespace wukong;

bool DemoDBUtils::LoadProfile(RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas) {
    serverId = 0;
    pDatas.clear();
    // 先从cache中加载profile数据
    redisContext *cache = g_CachePool.take();
    if (!cache) {
        ERROR_LOG("DemoDBUtils::LoadProfile -- connect to cache failed\n");
        return false;
    }

    switch (RedisUtils::LoadProfile(cache, roleId, serverId, pDatas)) {
        case REDIS_SUCCESS: {
            g_CachePool.put(cache, false);
            return true;
        }
        case REDIS_DB_ERROR: {
            g_CachePool.put(cache, true);
            ERROR_LOG("DemoDBUtils::LoadProfile -- load role profile failed for db error\n");
            return false;
        }
    }

    assert(pDatas.size() == 0);

    std::list<std::pair<std::string, std::string>> rDatas;

    // cache中找不到profile数据，先从cache中尝试加载角色数据并存入cache中
    if (RedisUtils::LoadRole(cache, roleId, serverId, rDatas, false) == REDIS_DB_ERROR) {
        g_CachePool.put(cache, true);
        ERROR_LOG("DemoDBUtils::LoadProfile -- load role data failed for db error\n");
        return false;
    }
    
    if (rDatas.size() > 0) {
        DemoUtils::MakeProfile(rDatas, pDatas);

        switch (RedisUtils::SaveProfile(cache, roleId, serverId, pDatas)) {
            case REDIS_DB_ERROR: {
                g_CachePool.put(cache, true);
                ERROR_LOG("DemoDBUtils::LoadProfile -- save role profile failed for db error\n");
                return false;
            }
            case REDIS_FAIL: {
                g_CachePool.put(cache, false);
                ERROR_LOG("DemoDBUtils::LoadProfile -- save role profile failed\n");
                return false;
            }
        }

        g_CachePool.put(cache, false);
        return true;
    }
    
    g_CachePool.put(cache, false);

    // cache中没有数据，需要从mysql中加载数据并存入cache中
    MYSQL *mysql = g_MysqlPool.take();
    if (!mysql) {
        ERROR_LOG("DemoDBUtils::LoadProfile -- connect to mysql failed\n");
        return false;
    }

    std::string data;
    if (!MysqlUtils::LoadRole(mysql, roleId, serverId, data)) {
        g_MysqlPool.put(mysql, true);
        return false;
    }

    g_MysqlPool.put(mysql, false);

    if (data.empty()) {
        ERROR_LOG("DemoDBUtils::LoadProfile -- no role data\n");
        return false;
    }

    wukong::pb::DataFragments fragments;
    if (!fragments.ParseFromString(data)) {
        ERROR_LOG("DemoDBUtils::LoadProfile -- parse role:%d data failed\n", roleId);
        return false;
    }

    int fragNum = fragments.fragments_size();
    for (int i = 0; i < fragNum; i++) {
        auto &fragment = fragments.fragments(i);
        pDatas.push_back(std::make_pair(fragment.fragname(), fragment.fragdata()));
    }

    if (rDatas.size() == 0) {
        return false;
    }

    DemoUtils::MakeProfile(rDatas, pDatas);

    cache = g_CachePool.take();
    if (!cache) {
        ERROR_LOG("DemoDBUtils::LoadProfile -- connect to cache failed\n");
        return false;
    }

    switch (RedisUtils::SaveProfile(cache, roleId, serverId, pDatas)) {
        case REDIS_DB_ERROR: {
            g_CachePool.put(cache, true);
            ERROR_LOG("DemoDBUtils::LoadProfile -- save role profile failed for db error\n");
            return false;
        }
        case REDIS_FAIL: {
            g_CachePool.put(cache, false);
            ERROR_LOG("DemoDBUtils::LoadProfile -- save role profile failed\n");
            return false;
        }
    }

    g_CachePool.put(cache, false);
    return true;
}
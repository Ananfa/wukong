// TODO：此文件通过工具生成

#include "demo_utils.h"

bool DemoUtils::LoadProfile(RedisConnectPool *cachePool, MysqlConnectPool *mysqlPool, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas) {
	serverId = 0;
    pDatas.clear();
    // 先从cache中加载profile数据
    redisContext *cache = cachePool->proxy.take();
    if (!cache) {
        ERROR_LOG("DemoUtils::LoadProfile -- connect to cache failed\n");
        return false;
    }

    if (!RedisUtils::LoadProfile(cache, roleId, serverId, pDatas)) {
        cachePool->proxy.put(cache, true);
        ERROR_LOG("DemoUtils::LoadProfile -- load role profile failed\n");
        return false;
    }

    cachePool->proxy.put(cache, false);
    
    if (pDatas.size() > 0) {
        return true;
    }
    
    // cache中没有数据，需要从mysql中加载数据并存入cache中
    MYSQL *mysql = mysqlPool->proxy.take();
    if (!mysql) {
        ERROR_LOG("DemoUtils::LoadProfile -- connect to mysql failed\n");
        return false;
    }

    std::string data;
    if (!MysqlUtils::LoadRoleData(mysql, roleId, serverId, data)) {
        mysqlPool->proxy.put(mysql, true);
        return false;
    }

    mysqlPool->proxy.put(mysql, false);

    if (data.empty()) {
        ERROR_LOG("DemoUtils::LoadProfile -- no role data\n");
        return false;
    }

    std::list<std::pair<std::string, std::string>> rDatas;
    wukong::pb::DataFragments fragments;
    if (!fragments.ParseFromString(data)) {
        ERROR_LOG("DemoUtils::LoadProfile -- parse role:%d data failed\n", roleId);
        return false;
    }

    int fragNum = fragments.fragments_size();
    for (int i = 0; i < fragNum; i++) {
        auto &fragment = fragments.fragments(i);
        pDatas.push_back(std::make_pair(fragment.fragName(), fragment.fragdata()));
    }

    if (rDatas.size() == 0) {
        return false;
    }

    // 注意：下面这段循环在不同生成代码中会不一样
    for (auto &rData : rDatas) {
        if (rData.first().compare("name") == 0 || 
            rData.first().compare("lv") == 0) {
            pDatas.push_back(rData);
        }
    }

    cache = cachePool->proxy.take();
    if (!cache) {
        ERROR_LOG("DemoUtils::LoadProfile -- connect to cache failed\n");
        return false;
    }

    if (!RedisUtils::SaveProfile(cache, roleId, serverId, pDatas)) {
        cachePool->proxy.put(cache, true);
        ERROR_LOG("DemoUtils::LoadProfile -- save role profile failed\n");
        return false;
    }

    cachePool->proxy.put(cache, false);
    return true;
}

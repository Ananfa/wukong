// TODO：此文件通过工具生成

#include "demo_utils.h"
#include "redis_utils.h"
#include "mysql_utils.h"
#include "common.pb.h"

using namespace demo;
using namespace wukong;

bool DemoUtils::LoadProfile(RedisConnectPool *cachePool, MysqlConnectPool *mysqlPool, const std::string &loadRoleSha1, const std::string &saveProfileSha1, RoleId roleId, ServerId &serverId, std::list<std::pair<std::string, std::string>> &pDatas) {
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

    if (pDatas.size() > 0) {
        cachePool->proxy.put(cache, false);
        return true;
    }

    std::list<std::pair<std::string, std::string>> rDatas;

    // cache中找不到profile数据，先从cache中尝试加载角色数据并存入cache中
    if (!RedisUtils::LoadRole(cache, loadRoleSha1, roleId, serverId, rDatas, false)) {
        cachePool->proxy.put(cache, true);
        ERROR_LOG("DemoUtils::LoadProfile -- load role data failed\n");
        return false;
    }
    
    if (rDatas.size() > 0) {
        MakeProfile(rDatas, pDatas);

        if (!RedisUtils::SaveProfile(cache, saveProfileSha1, roleId, serverId, pDatas)) {
            cachePool->proxy.put(cache, true);
            ERROR_LOG("DemoUtils::LoadProfile -- save role profile failed\n");
            return false;
        }

        cachePool->proxy.put(cache, false);
        return true;
    }
    
    cachePool->proxy.put(cache, false);

    // cache中没有数据，需要从mysql中加载数据并存入cache中
    MYSQL *mysql = mysqlPool->proxy.take();
    if (!mysql) {
        ERROR_LOG("DemoUtils::LoadProfile -- connect to mysql failed\n");
        return false;
    }

    std::string data;
    if (!MysqlUtils::LoadRole(mysql, roleId, serverId, data)) {
        mysqlPool->proxy.put(mysql, true);
        return false;
    }

    mysqlPool->proxy.put(mysql, false);

    if (data.empty()) {
        ERROR_LOG("DemoUtils::LoadProfile -- no role data\n");
        return false;
    }

    wukong::pb::DataFragments fragments;
    if (!fragments.ParseFromString(data)) {
        ERROR_LOG("DemoUtils::LoadProfile -- parse role:%d data failed\n", roleId);
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

    MakeProfile(rDatas, pDatas);

    cache = cachePool->proxy.take();
    if (!cache) {
        ERROR_LOG("DemoUtils::LoadProfile -- connect to cache failed\n");
        return false;
    }

    if (!RedisUtils::SaveProfile(cache, saveProfileSha1, roleId, serverId, pDatas)) {
        cachePool->proxy.put(cache, true);
        ERROR_LOG("DemoUtils::LoadProfile -- save role profile failed\n");
        return false;
    }

    cachePool->proxy.put(cache, false);
    return true;
}

void DemoUtils::MakeProfile(const std::list<std::pair<std::string, std::string>> &datas, std::list<std::pair<std::string, std::string>> &pDatas) {
    // 注意：下面这段循环在不同生成代码中会不一样
    for (auto &data : datas) {
        if (data.first.compare("name") == 0 || 
            data.first.compare("lv") == 0) {
            pDatas.push_back(data);
        }
    }
}
/*
 * Created by Xianke Liu on 2021/5/6.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "record_center.h"
#include "redis_pool.h"
#include "mysql_pool.h"
#include "redis_utils.h"
#include "mysql_utils.h"
#include "proto_utils.h"
#include "share/const.h"

using namespace wukong;

void RecordCenter::init() {
    //_cache = corpc::RedisConnectPool::create(g_RecordConfig.getCache().host.c_str(), g_RecordConfig.getCache().port, g_RecordConfig.getCache().dbIndex, g_RecordConfig.getCache().maxConnect);
    //_mysql = corpc::MysqlConnectPool::create(g_RecordConfig.getMysql().host.c_str(), g_RecordConfig.getMysql().user.c_str(), g_RecordConfig.getMysql().pwd.c_str(), g_RecordConfig.getMysql().dbName.c_str(), g_RecordConfig.getMysql().port, "", 0, g_RecordConfig.getMysql().maxConnect);

    // 定时落地协程
    RoutineEnvironment::startCoroutine(saveRoutine, this);
}

void *RecordCenter::saveRoutine(void *arg) {
    RecordCenter *self = (RecordCenter *)arg;

    uint64_t lastSec = 0;
    struct timeval t;
    // 每秒对当前秒对应的SET上锁（锁超时1分钟），读取整个SET，将SET中所有玩家数据存盘，然后清除SET，等超时自动解锁
    while (true) {
        gettimeofday(&t, NULL);
        if (t.tv_sec == lastSec) {
            msleep(200);
            continue;
        }
        lastSec = t.tv_sec;

        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("RecordCenter::saveRoutine -- connect to cache failed\n");
            continue;
        }

        gettimeofday(&t, NULL);
        uint32_t wheelPos = t.tv_sec % SAVE_TIME_WHEEL_SIZE;

        // 当前实现是每个Record服抢占一秒的集合进行处理，是否可以用redis集合的spop来实现逻辑提高存盘性能？
        // pop出来的玩家ID要是存盘失败就会导致数据没有存盘怎么办？应该是存盘完成后才能删除，或者当没成功存盘时写回集合中
        // pop出来处理中途服务器关闭导致数据没存盘怎么办？应该是存盘完成后才能删除
        // 从上面分析，目前采用抢占集合方式进行处理相对可靠，通过并发启动多个存盘工作协程方式来提高存盘效率

        switch (RedisUtils::SaveLock(cache, wheelPos)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("RecordCenter::saveRoutine -- redis reply null\n");
                continue;
            }
            case REDIS_FAIL: {
                g_RedisPoolManager.getCoreCache()->put(cache, false);
                continue;
            }
        }

        std::vector<RoleId> roleIds;
        switch (RedisUtils::GetSaveList(cache, wheelPos, roleIds)) {
            case REDIS_DB_ERROR: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("RecordCenter::saveRoutine -- redis reply null\n");
                continue;
            }
            case REDIS_FAIL: {
                g_RedisPoolManager.getCoreCache()->put(cache, true);
                ERROR_LOG("RecordCenter::saveRoutine -- redis reply type invalid\n");
                continue;
            }
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
        
        for (RoleId roleId : roleIds){
            // 开工作协程并发存盘
            self->_saveSema.wait();

            WorkerTask *task = new WorkerTask;
            task->center = self;
            task->roleId = roleId;
            task->wheelPos = wheelPos;
            RoutineEnvironment::startCoroutine(saveWorkerRoutine, task);
        }
    }
}

void *RecordCenter::saveWorkerRoutine(void *arg) {
    WorkerTask *task = (WorkerTask *)arg;
    RecordCenter *self = task->center;
    RoleId roleId = task->roleId;
    uint32_t wheelPos = task->wheelPos;
    delete task;

    // 先从cache中加载profile数据
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("DemoUtils::SaveRole -- connect to cache failed\n");
        self->_saveSema.post();
        return nullptr;
    }

    std::list<std::pair<std::string, std::string>> datas;
    ServerId serverId;
    if (RedisUtils::LoadRole(cache, roleId, serverId, datas, false) == REDIS_DB_ERROR) {
        g_RedisPoolManager.getCoreCache()->put(cache, true);
        ERROR_LOG("DemoUtils::SaveRole -- load role data failed\n");
        self->_saveSema.post();
        return nullptr;
    }

    if (datas.size() > 0) {
        // 先暂时归还cache连接
        g_RedisPoolManager.getCoreCache()->put(cache, false);

        // 保存到mysql中
        MYSQL *mysql = g_MysqlPoolManager.getCoreRecord()->take();
        if (!mysql) {
            ERROR_LOG("DemoUtils::SaveRole -- connect to mysql failed\n");
            
            self->_saveSema.post();
            return nullptr;
        }

        std::string roleData = ProtoUtils::marshalDataFragments(datas);
        if (!MysqlUtils::UpdateRole(mysql, roleId, roleData)) {
            ERROR_LOG("DemoUtils::SaveRole -- save to mysql failed\n");
            g_MysqlPoolManager.getCoreRecord()->put(mysql, true);
            
            self->_saveSema.post();
            return nullptr;
        }

        g_MysqlPoolManager.getCoreRecord()->put(mysql, false);
        
        // 重新获取cache连接
        cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("DemoUtils::SaveRole -- connect to cache failed when remove id from set\n");
            
            self->_saveSema.post();
            return nullptr;
        }
    } else {
        // cache中没有数据，不需要保存
        WARN_LOG("DemoUtils::SaveRole -- load role data empty\n");
    }

    // 删除集合中玩家ID，这里一个个玩家ID单独从集合中删除而不是在最后删除集合是防止处理集合过程中集合插入新的ID
    if (RedisUtils::RemoveSaveRoleId(cache, wheelPos, roleId) == REDIS_DB_ERROR) {
        g_RedisPoolManager.getCoreCache()->put(cache, true);
        ERROR_LOG("RecordCenter::saveRoutine -- redis reply null when remove id from set\n");
        
        self->_saveSema.post();
        return nullptr;
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);

    self->_saveSema.post();
    return nullptr;
}
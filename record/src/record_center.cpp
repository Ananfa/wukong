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
#include "cache_pool.h"
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

        redisContext *cache = g_CachePool.take();
        if (!cache) {
            ERROR_LOG("RecordCenter::saveRoutine -- connect to cache failed\n");
            continue;
        } else {
            gettimeofday(&t, NULL);
            uint32_t wheelPos = t.tv_sec % SAVE_TIME_WHEEL_SIZE;

            // 当前实现是每个Record服抢占一秒的集合进行处理，是否可以用redis集合的spop来实现逻辑提高存盘性能？
            // pop出来的玩家ID要是存盘失败就会导致数据没有存盘怎么办？应该是存盘完成后才能删除，或者当没成功存盘时写回集合中
            // pop出来处理中途服务器关闭导致数据没存盘怎么办？应该是存盘完成后才能删除
            // 从上面分析，目前采用抢占集合方式进行处理相对可靠，通过并发启动多个存盘工作协程方式来提高存盘效率

            redisReply *reply = (redisReply *)redisCommand(cache, "SET SaveLock:%d 1 NX EX 60", wheelPos);
            if (!reply) {
                g_CachePool.put(cache, true);
                ERROR_LOG("RecordCenter::saveRoutine -- redis reply null\n");
                continue;
            } else if (strcmp(reply->str, "OK")) {
                // 上锁不成功
                freeReplyObject(reply);
                g_CachePool.put(cache, false);
                continue;
            }

            freeReplyObject(reply);

            // 读取整个SET
            reply = (redisReply *)redisCommand(cache, "SMEMBERS Save:%d", wheelPos);
            if (!reply) {
                g_CachePool.put(cache, true);
                ERROR_LOG("RecordCenter::saveRoutine -- redis reply null\n");
                continue;
            }

            if (reply->type != REDIS_REPLY_ARRAY) {
                freeReplyObject(reply);
                g_CachePool.put(cache, true);
                ERROR_LOG("RecordCenter::saveRoutine -- redis reply type invalid\n");
                continue;
            }

            std::list<RoleId> roleIds;
            if (reply->elements > 0) {
                for (int i = 0; i < reply->elements; i++) {
                    RoleId roleId = atoi(reply->element[i]->str);
                    if (roleId == 0) {
                        ERROR_LOG("RecordCenter::saveRoutine -- invalid roleid\n");
                        continue;
                    }

                    roleIds.push_back(roleId);
                }
            }

            freeReplyObject(reply);
            g_CachePool.put(cache, false);
            
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
}

void *RecordCenter::saveWorkerRoutine(void *arg) {
    WorkerTask *task = (WorkerTask *)arg;
    RecordCenter *self = task->center;
    RoleId roleId = task->roleId;
    uint32_t wheelPos = task->wheelPos;
    delete task;

    // 先从cache中加载profile数据
    redisContext *cache = g_CachePool.take();
    if (!cache) {
        ERROR_LOG("DemoUtils::SaveRole -- connect to cache failed\n");
        self->_saveSema.post();
        return nullptr;
    }

    std::list<std::pair<std::string, std::string>> datas;
    ServerId serverId;
    if (RedisUtils::LoadRole(cache, roleId, serverId, datas, false) == REDIS_DB_ERROR) {
        g_CachePool.put(cache, true);
        ERROR_LOG("DemoUtils::SaveRole -- load role data failed\n");
        self->_saveSema.post();
        return nullptr;
    }

    if (datas.size() == 0) {
        // cache中没有数据，不需要保存
        g_CachePool.put(cache, false);
        WARN_LOG("DemoUtils::SaveRole -- load role data empty\n");
    } else {
        g_CachePool.put(cache, false);

        // 保存到mysql中
        MYSQL *mysql = g_MysqlPool.take();
        if (!mysql) {
            ERROR_LOG("DemoUtils::SaveRole -- connect to mysql failed\n");
            
            self->_saveSema.post();
            return nullptr;
        }

        std::string roleData = ProtoUtils::marshalDataFragments(datas);
        if (!MysqlUtils::UpdateRole(mysql, roleId, roleData)) {
            ERROR_LOG("DemoUtils::SaveRole -- save to mysql failed\n");
            g_MysqlPool.put(mysql, true);
            
            self->_saveSema.post();
            return nullptr;
        }

        g_MysqlPool.put(mysql, false);
    }

    // 删除集合中玩家ID，这里一个个玩家ID单独从集合中删除而不是在最后删除集合是防止处理集合过程中集合插入新的ID
    cache = g_CachePool.take();
    if (!cache) {
        ERROR_LOG("DemoUtils::SaveRole -- connect to cache failed when remove id from set\n");
        
        self->_saveSema.post();
        return nullptr;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "SREM Save:%d %d", wheelPos, roleId);
    if (!reply) {
        g_CachePool.put(cache, true);
        ERROR_LOG("RecordCenter::saveRoutine -- redis reply null when remove id from set\n");
        
        self->_saveSema.post();
        return nullptr;
    }

    freeReplyObject(reply);
    g_CachePool.put(cache, false);

    self->_saveSema.post();
    return nullptr;
}
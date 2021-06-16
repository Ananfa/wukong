/*
 * Created by Xianke Liu on 2021/5/17.
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

#include "corpc_routine_env.h"
#include "record_object.h"
#include "record_center.h"
#include "record_manager.h"
#include "redis_utils.h"
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

RecordObject::~RecordObject() {}

void RecordObject::start() {
    _running = true;

    {
        RecordObjectRoutineArg *arg = new RecordObjectRoutineArg();
        arg->obj = shared_from_this();
        RoutineEnvironment::startCoroutine(heartbeatRoutine, arg);
    }

    // 启动定期存盘协程（每隔一段时间将脏数据写入redis，协程中退出循环时也会执行一次）
    {
        RecordObjectRoutineArg *arg = new RecordObjectRoutineArg();
        arg->obj = shared_from_this();
        RoutineEnvironment::startCoroutine(syncRoutine, arg);
    }
}

void RecordObject::stop() {
    _running = false;

    // 设置cache数据超时时间
    redisContext *cache = g_RecordCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("RecordObject::stop -- role %d connect to cache failed\n", _roleId);
        return;
    }

    redisReply *reply = (redisReply *)redisCommand(cache, "EXPIRE Role:{%d} %d", _roleId, RECORD_EXPIRE);
    if (!reply) {
        g_RecordCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("RecordObject::stop -- role %d expire cache data failed for db error\n", _roleId);
        return;
    }

    freeReplyObject(reply);
    g_RecordCenter.getCachePool()->proxy.put(cache, false);
}

void *RecordObject::heartbeatRoutine( void *arg ) {
    RecordObjectRoutineArg* routineArg = (RecordObjectRoutineArg*)arg;
    std::shared_ptr<RecordObject> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;
    gettimeofday(&t, NULL);

    obj->_gameObjectHeartbeatExpire = t.tv_sec + RECORD_TIMEOUT;

    while (obj->_running) {
        sleep(TOKEN_HEARTBEAT_PERIOD); // 游戏对象销毁后，心跳协程最多停留20秒（这段时间会占用一点系统资源）

        if (!obj->_running) {
            // 游戏对象已被销毁
            break;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_RecordCenter.getCachePool()->proxy.take();
        if (!cache) {
            ERROR_LOG("RecordObject::heartbeatRoutine -- role %d connect to cache failed\n", obj->_roleId);

            success = false;
        } else {
            redisReply *reply;
            if (g_RecordCenter.setRecordExpireSha1().empty()) {
                reply = (redisReply *)redisCommand(cache, "EVAL %s 1 record:%d %d %d", SET_RECORD_EXPIRE_CMD, obj->_roleId, obj->_rToken, TOKEN_TIMEOUT);
            } else {
                reply = (redisReply *)redisCommand(cache, "EVALSHA %s 1 record:%d %d %d", g_RecordCenter.setRecordExpireSha1(), obj->_roleId, obj->_rToken, TOKEN_TIMEOUT);
            }
            
            if (!reply) {
                g_RecordCenter.getCachePool()->proxy.put(cache, true);
                ERROR_LOG("RecordObject::heartbeatRoutine -- role %d check session failed for db error\n", obj->_roleId);

                success = false;
            } else {
                success = reply->integer == 1;
                freeReplyObject(reply);
                g_RecordCenter.getCachePool()->proxy.put(cache, false);

                if (!success) {
                    ERROR_LOG("RecordObject::heartbeatRoutine -- role %d check session failed\n", obj->_roleId);
                }
            }
        }

        if (success) {
            // 判断游戏对象心跳是否过期
            gettimeofday(&t, NULL);
            success = t.tv_sec < obj->_gameObjectHeartbeatExpire;
            if (!success) {
                ERROR_LOG("RecordObject::heartbeatRoutine -- role %d heartbeat expired\n", obj->_roleId);
            }
        }

        // 若设置超时不成功，销毁记录对象
        if (!success) {
            if (obj->_running) {
                if (!obj->_manager->remove(obj->_roleId)) {
                    assert(false);
                    ERROR_LOG("RecordObject::heartbeatRoutine -- role %d remove record object failed\n", obj->_roleId);
                }

                obj->_running = false;
            }
        }
    }

    return nullptr;
}

void *RecordObject::syncRoutine(void *arg) {
    RecordObjectRoutineArg* routineArg = (RecordObjectRoutineArg*)arg;
    std::shared_ptr<RecordObject> obj = std::move(routineArg->obj);
    delete routineArg;

    // 每隔一段时间将脏数据存到redis数据库
    std::list<std::pair<std::string, std::string>> syncDatas;
    std::list<std::pair<std::string, std::string>> profileDatas;

    while (obj->_running) {
        for (int i = 0; i < CACHE_PERIOD; i++) {
            sleep(1);

            if (!obj->_running) {
                break;
            }
        }

        // 向记录服同步数据（销毁前也应将脏数据存盘）
        obj->buildSyncDatas(syncDatas);
        if (!syncDatas.empty()) {
            if (obj->cacheData(syncDatas)) {
                obj->_dirty_map.clear();
                obj->_cacheFailNum = 0;
            } else {
                // 增加失败计数，若连续3次失败则销毁记录对象，防止长时间回档
                ERROR_LOG("RecordObject::syncRoutine -- role %d cache data failed\n", obj->_roleId);
                ++obj->_cacheFailNum;
                if (obj->_cacheFailNum > 3) {
                    obj->stop();
                }
            }

            syncDatas.clear();
        }

        g_RecordCenter.getMakeProfileHandle()(syncDatas, profileDatas);
        if (!profileDatas.empty()) {
            if (!obj->cacheProfile(profileDatas)) {
                ERROR_LOG("RecordObject::syncRoutine -- role %d save profile failed\n", obj->_roleId);
            }

            profileDatas.clear();
        }
    }

    return nullptr;
}

bool RecordObject::cacheData(std::list<std::pair<std::string, std::string>> &datas) {
    // 将数据存到cache中，并且加入相应的存盘时间队列
    redisContext *cache = g_RecordCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("RecordObject::cacheData -- role %d connect to cache failed\n", _roleId);

        return false;
    }

    if (!RedisUtils::UpdateRole(cache, g_RecordCenter.updateRoleSha1(), _roleId, datas)) {
        g_RecordCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("RecordObject::cacheData -- role %d update data failed\n", _roleId);
        return false;
    }

    // 加入相应的落地队列，等待落地任务将玩家数据存到mysql。当数据有修改过5分钟再进行落地
    struct timeval t;
    gettimeofday(&t, NULL);
    uint64_t nowTM = t.tv_sec;
    if (nowTM >= _saveTM) {
        uint64_t nextSaveTM = nowTM + SAVE_PERIOD;
        redisReply *reply = (redisReply *)redisCommand(cache, "SADD save:%d %d", nextSaveTM, _roleId);
        if (!reply) {
            g_RecordCenter.getCachePool()->proxy.put(cache, true);
            WARN_LOG("RecordObject::cacheData -- role %d insert save list failed for db error\n", _roleId);
        } else {
            _saveTM = nextSaveTM;
            freeReplyObject(reply);
            g_RecordCenter.getCachePool()->proxy.put(cache, false);
        }
    } else {
        g_RecordCenter.getCachePool()->proxy.put(cache, false);
    }

    return true;
}

bool RecordObject::cacheProfile(std::list<std::pair<std::string, std::string>> &profileDatas) {
    redisContext *cache = g_RecordCenter.getCachePool()->proxy.take();
    if (!cache) {
        ERROR_LOG("RecordObject::cacheProfile -- role %d connect to cache failed\n", _roleId);
        return false;
    }

    if (!RedisUtils::UpdateProfile(cache, g_RecordCenter.updateProfileSha1(), _roleId, profileDatas)) {
        g_RecordCenter.getCachePool()->proxy.put(cache, true);
        ERROR_LOG("RecordObject::cacheProfile -- role %d update profile failed\n", _roleId);
        return false;
    }

    g_RecordCenter.getCachePool()->proxy.put(cache, false);
    return true;
}

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
#include "record_object_manager.h"
#include "redis_pool.h"
//#include "record_delegate.h"
#include "redis_utils.h"
#include "share/const.h"
#include "demo_utils.h"

#include <sys/time.h>

using namespace wukong;
using namespace demo;

RecordObject::~RecordObject() {
    DEBUG_LOG("RecordObject::~RecordObject -- role[%d]\n", roleId_);
}

void RecordObject::syncIn(const ::wukong::pb::SyncRequest* request) {
    data_->syncIn(request);
}

void RecordObject::buildSyncDatas(std::list<std::pair<std::string, std::string>> &datas) {
    data_->buildSyncDatas(datas);
}

void RecordObject::buildAllDatas(std::list<std::pair<std::string, std::string>> &datas) {
    data_->buildAllDatas(datas);
}

void RecordObject::clearDirty() {
    data_->clearDirty();
}

void RecordObject::start() {
    running_ = true;

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
    if (running_) {
        running_ = false;

        cond_.broadcast();

        // TODO: 在这里直接进行redis操作会有协程切换，导致一些流程同步问题，需要考虑一下是否需要换地方调用
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("RecordObject::stop -- role[%d] connect to cache failed\n", roleId_);
            return;
        }

        if (RedisUtils::RemoveRecordAddress(cache, roleId_, rToken_) == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("RecordObject::stop -- role[%d] remove record failed", roleId_);
            return;
        }

        if (RedisUtils::SetRoleTTL(cache, roleId_) == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("RecordObject::stop -- role[%d] expire cache data failed for db error\n", roleId_);
            return;
        }

        g_RedisPoolManager.getCoreCache()->put(cache, false);
    }
}

void *RecordObject::heartbeatRoutine( void *arg ) {
    RecordObjectRoutineArg* routineArg = (RecordObjectRoutineArg*)arg;
    std::shared_ptr<RecordObject> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;
    gettimeofday(&t, NULL);

    obj->gameObjectHeartbeatExpire_ = t.tv_sec + RECORD_TIMEOUT;

    while (obj->running_) {
        // 目前这里只有当写数据库失败强退记录对象时才会触发（记录对象销毁时机：1.心跳失败 2.写数据库失败）
        obj->cond_.wait(TOKEN_HEARTBEAT_PERIOD);

        if (!obj->running_) {
            // 游戏对象已被销毁
            break;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
        if (!cache) {
            ERROR_LOG("RecordObject::heartbeatRoutine -- role %d connect to cache failed\n", obj->roleId_);

            success = false;
        } else {
            switch (RedisUtils::SetRecordAddressTTL(cache, obj->roleId_, obj->rToken_)) {
                case REDIS_DB_ERROR: {
                    g_RedisPoolManager.getCoreCache()->put(cache, true);
                    ERROR_LOG("RecordObject::heartbeatRoutine -- role %d check session failed for db error\n", obj->roleId_);
                    success = false;
                }
                case REDIS_FAIL: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    ERROR_LOG("RecordObject::heartbeatRoutine -- role %d check session failed\n", obj->roleId_);
                    success = false;
                }
                case REDIS_SUCCESS: {
                    g_RedisPoolManager.getCoreCache()->put(cache, false);
                    success = true;
                }
            }
        }

        if (success) {
            // 判断游戏对象心跳是否过期
            gettimeofday(&t, NULL);
            success = t.tv_sec < obj->gameObjectHeartbeatExpire_;
            if (!success) {
                ERROR_LOG("RecordObject::heartbeatRoutine -- role %d heartbeat expired\n", obj->roleId_);
            }
        }

        // 若设置超时不成功，销毁记录对象
        if (!success) {
            if (obj->running_) {
                if (!g_RecordObjectManager.remove(obj->roleId_)) {
                    assert(false);
                    ERROR_LOG("RecordObject::heartbeatRoutine -- role %d remove record object failed\n", obj->roleId_);
                }

                obj->running_ = false;
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

    while (obj->running_) {
        // 目前这里只有当心跳失败时才会被触发（记录对象销毁时机：1.心跳失败 2.写数据库失败）
        obj->cond_.wait(CACHE_PERIOD);

        if (!obj->running_) {
            break;
        }

        // 向记录服同步数据（销毁前也应将脏数据存盘）
        obj->buildSyncDatas(syncDatas);
        if (!syncDatas.empty()) {
            if (obj->cacheData(syncDatas)) {
                obj->clearDirty();
                obj->cacheFailNum_ = 0;
            } else {
                // 增加失败计数，若连续3次失败则销毁记录对象，防止长时间回档
                ERROR_LOG("RecordObject::syncRoutine -- role %d cache data failed\n", obj->roleId_);
                ++obj->cacheFailNum_;
                if (obj->cacheFailNum_ > 3) {
                    g_RecordObjectManager.remove(obj->roleId_);
                }
            }

            syncDatas.clear();
        }

        obj->makeProfile(syncDatas, profileDatas);
        if (!profileDatas.empty()) {
            if (!obj->cacheProfile(profileDatas)) {
                ERROR_LOG("RecordObject::syncRoutine -- role %d save profile failed\n", obj->roleId_);
            }

            profileDatas.clear();
        }
    }

    return nullptr;
}

bool RecordObject::cacheData(std::list<std::pair<std::string, std::string>> &datas) {
    // 将数据存到cache中，并且加入相应的存盘时间队列
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("RecordObject::cacheData -- role %d connect to cache failed\n", roleId_);

        return false;
    }

    switch (RedisUtils::UpdateRole(cache, roleId_, datas)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("RecordObject::cacheData -- role %d update data failed for db error\n", roleId_);
            return false;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("RecordObject::cacheData -- role %d update data failed\n", roleId_);
            return false;
        }
    }

    // 加入相应的落地时间轮中，等待落地任务将玩家数据存到mysql。当数据有修改过5分钟再进行落地
    struct timeval t;
    gettimeofday(&t, NULL);
    uint64_t nowTM = t.tv_sec;
    if (nowTM >= saveTM_) {
        // 让玩家的存盘落在固定的时间轮位置上，减少不必要存盘
        uint32_t refPos = roleId_ % SAVE_PERIOD;
        // 计算当前时间往后第一个能模SAVE_PERIOD等于refPos的时间点
        uint64_t nextSaveTM = (nowTM / SAVE_PERIOD) * SAVE_PERIOD + refPos;
        if (nextSaveTM <= nowTM) {
            nextSaveTM += SAVE_PERIOD;
        }

        uint32_t whealPos = nextSaveTM % SAVE_TIME_WHEEL_SIZE;

        if (RedisUtils::AddSaveRoleId(cache, whealPos, roleId_) == REDIS_DB_ERROR) {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            WARN_LOG("RecordObject::cacheData -- role %d insert save list failed for db error\n", roleId_);
        } else {
            saveTM_ = nextSaveTM;
            g_RedisPoolManager.getCoreCache()->put(cache, false);
        }
    } else {
        g_RedisPoolManager.getCoreCache()->put(cache, false);
    }

    return true;
}

bool RecordObject::cacheProfile(std::list<std::pair<std::string, std::string>> &profileDatas) {
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("RecordObject::cacheProfile -- role %d connect to cache failed\n", roleId_);
        return false;
    }

    switch (RedisUtils::UpdateProfile(cache, roleId_, profileDatas)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("RecordObject::cacheProfile -- role %d update profile failed for db error\n", roleId_);
            return false;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, false);
            ERROR_LOG("RecordObject::cacheProfile -- role %d update profile failed\n", roleId_);
            return false;
        }
    }

    g_RedisPoolManager.getCoreCache()->put(cache, false);
    return true;
}

void RecordObject::makeProfile(const std::list<std::pair<std::string, std::string>> &syncDatas, std::list<std::pair<std::string, std::string>> &profileDatas) {
    DemoUtils::MakeProfile(syncDatas, profileDatas);
}

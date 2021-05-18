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
#include "share/const.h"

#include <sys/time.h>

using namespace wukong;

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
}

void *RecordObject::heartbeatRoutine( void *arg ) {
    RecordObjectRoutineArg* routineArg = (RecordObjectRoutineArg*)arg;
    std::shared_ptr<RecordObject> obj = std::move(routineArg->obj);
    delete routineArg;

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
                reply = (redisReply *)redisCommand(cache, "eval %s 1 record:%d %d %d", SET_RECORD_EXPIRE_CMD, obj->_roleId, obj->_rToken, TOKEN_TIMEOUT);
            } else {
                reply = (redisReply *)redisCommand(cache, "evalsha %s 1 record:%d %d %d", g_RecordCenter.setRecordExpireSha1(), obj->_roleId, obj->_rToken, TOKEN_TIMEOUT);
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

    while (obj->_running) {
        for (int i = 0; i < CACHE_PERIOD; i++) {
            sleep(1);

            if (!obj->_running) {
                break;
            }
        }

        // 向记录服同步数据（销毁前也应将脏数据存盘）
        obj->buildSyncDatas(syncDatas);
        if (!syncDatas.empty() || !removes.empty()) {
            if (obj->cacheData(syncDatas)) {
                obj->_dirty_map.clear();
            }

            syncDatas.clear();
        }

    }
}

void *RecordObject::updateRoutine(void *arg) {
    RecordObjectRoutineArg* routineArg = (RecordObjectRoutineArg*)arg;
    std::shared_ptr<RecordObject> obj = std::move(routineArg->obj);
    delete routineArg;

    struct timeval t;

    while (obj->_running) {
        msleep(g_GameCenter.getGameObjectUpdatePeriod());

        if (!obj->_running) {
            // 游戏对象已被销毁
            break;
        }

        gettimeofday(&t, NULL);
        obj->update(t.tv_sec);
    }
}

bool RecordObject::cacheData(std::list<std::pair<std::string, std::string>> &datas) {
    // TODO: 将数据存到cache中
}
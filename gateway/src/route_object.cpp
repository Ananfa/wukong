/*
 * Created by Xianke Liu on 2021/1/7.
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
#include "route_object.h"
#include "gateway_center.h"
#include "gateway_manager.h"
#include "share/const.h"

using namespace wukong;

void RouteObject::start() {
    _running = true;

    RouteObjectHeartbeatInfo *info = new RouteObjectHeartbeatInfo();
    info->ro = shared_from_this();

    RoutineEnvironment::startCoroutine(heartbeatRoutine, info);
}

void RouteObject::stop() {
    _running = false;
}

void *RouteObject::heartbeatRoutine( void *arg ) {
    RouteObjectHeartbeatInfo* info = (RouteObjectHeartbeatInfo*)arg;
    std::shared_ptr<RouteObject> ro = std::move(info->ro);
    delete info;

    while (ro->_running) {
        sleep(20); // 路由对象销毁后，心跳协程最多停留20秒（这段时间会占用一点系统资源）

        if (!ro->_running) {
            // 路由对象已被销毁
            return nullptr;
        }

        // 设置session超时
        bool success = true;
        redisContext *cache = g_GatewayCenter.getCachePool()->proxy.take();
        if (!cache) {
            ERROR_LOG("RouteObject::heartbeatRoutine -- user %d connect to cache failed\n", ro->_userId);

            success = false;
        } else {
            redisReply *reply;
            if (g_GatewayCenter.setSessionExpireSha1().empty()) {
                reply = (redisReply *)redisCommand(cache, "eval %s 1 session:%d %s %d", SET_SESSION_EXPIRE_CMD, ro->_userId, ro->_token.c_str(), 60);
            } else {
                reply = (redisReply *)redisCommand(cache, "eval %s 1 session:%d %s %d", g_GatewayCenter.setSessionExpireSha1(), ro->_userId, ro->_token.c_str(), 60);
            }
            
            if (!reply) {
                ERROR_LOG("RouteObject::heartbeatRoutine -- user %d check session failed for db error\n", ro->_userId);
                g_GatewayCenter.getCachePool()->proxy.put(cache, true);

                success = false;
            } else {
                success = reply->integer == 1;
                if (!success) {
                    ERROR_LOG("RouteObject::heartbeatRoutine -- user %d check session failed\n", ro->_userId);
                }
                freeReplyObject(reply);
                g_GatewayCenter.getCachePool()->proxy.put(cache, false);
            }
        }

        // 若设置超时不成功，销毁路由对象
        if (!success) {
            if (ro->_running) {
                if (!ro->_manager->removeRouteObject(ro->_userId)) {
                    assert(false);
                    ERROR_LOG("RouteObject::heartbeatRoutine -- user %d remove route object failed\n", ro->_userId);
                }

                ro->_running = false;
            }
        }
    }

    return nullptr;
}

/*
 * Created by Xianke Liu on 2020/12/22.
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
#include "gateway_manager.h"
#include "gateway_config.h"

#include <sys/time.h>

using namespace wukong;

void GatewayManager::init() {
    RoutineEnvironment::startCoroutine(clearExpiredUnauthRoutine, this);
    RoutineEnvironment::startCoroutine(clearExpiredDisconnectedRoutine, this);
}

void GatewayManager::shutdown() {
    _shutdown = true;

    // 销毁所有未认证连接
    clearUnauth();

    // 销毁所有路由对象
    clearRouteObject();
}

void GatewayManager::addUnauthConn(std::shared_ptr<MessageServer::Connection>& conn) {
    assert(_unauthNodeMap.find(conn.get()) == _unauthNodeMap.end());
    MessageConnectionTimeLink::Node *node = new MessageConnectionTimeLink::Node;
    node->data = conn;
    _unauthLink.push(node);
    
    _unauthNodeMap.insert(std::make_pair(conn.get(), node));
}

void GatewayManager::removeUnauthConn(std::shared_ptr<MessageServer::Connection>& conn) {
    auto kv = _unauthNodeMap.find(conn.get());
    if (kv == _unauthNodeMap.end()) {
        return;
    }
    
    MessageConnectionTimeLink::Node *node = kv->second;
    _unauthLink.erase(node);
    _unauthNodeMap.erase(kv);
}

bool GatewayManager::isUnauth(std::shared_ptr<MessageServer::Connection>& conn) {
    return _unauthNodeMap.find(conn.get()) != _unauthNodeMap.end();
}

void GatewayManager::clearUnauth() {
    MessageConnectionTimeLink::Node *node = _unauthLink.getHead();
    while (node) {
        node->data->close();
        node = node->next;
    }
    
    _unauthLink.clear();
    _unauthNodeMap.clear();
}

bool GatewayManager::hasRouteObject(UserId userId) {
    return _userId2RouteObjectMap.find(userId) != _userId2RouteObjectMap.end() || _disconnectedNodeMap.find(userId) != _disconnectedNodeMap.end();
}

bool GatewayManager::removeRouteObject(UserId userId) {
    auto it = _userId2RouteObjectMap.find(userId);
    if (it != _userId2RouteObjectMap.end()) {
        auto it1 = _connection2RouteObjectMap.find(it->second->getConn().get());
        assert(it1 != _connection2RouteObjectMap.end());
        it->second->getConn()->close();
        it->second->stop();
        _connection2RouteObjectMap.erase(it1);
        _userId2RouteObjectMap.erase(it);
        return true;
    }

    auto it2 = _disconnectedNodeMap.find(userId);
    if (it2 != _disconnectedNodeMap.end()) {
        it2->second->data->stop();
        _disconnectedLink.erase(it2->second);
        _disconnectedNodeMap.erase(it2);
        return true;
    }

    return false;
}

size_t GatewayManager::getRouteObjectNum() {
    return _userId2RouteObjectMap.size() + _disconnectedNodeMap.size();
}

void GatewayManager::clearRouteObject() {
    // 销毁所有正常routeObject
    for (auto it = _userId2RouteObjectMap.begin(); it != _userId2RouteObjectMap.end(); it++) {
        it->second->getConn()->close();
        it->second->stop();
    }
    _userId2RouteObjectMap.clear();
    _connection2RouteObjectMap.clear();

    // 销毁所有断线routeObject
    for (auto it = _disconnectedNodeMap.begin(); it != _disconnectedNodeMap.end(); it++) {
        it->second->data->stop();
    }
    _disconnectedNodeMap.clear();
    _disconnectedLink.clear();
}

std::shared_ptr<RouteObject> GatewayManager::getRouteObject(UserId userId) {
    auto it = _userId2RouteObjectMap.find(userId);
    if (it != _userId2RouteObjectMap.end()) {
        return it->second;
    }

    auto it2 = _disconnectedNodeMap.find(userId);
    if (it2 != _disconnectedNodeMap.end()) {
        return it2->second->data;
    }

    return nullptr;
}

std::shared_ptr<RouteObject> GatewayManager::getConnectedRouteObject(UserId userId) {
    auto it = _userId2RouteObjectMap.find(userId);
    if (it == _userId2RouteObjectMap.end()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<RouteObject> GatewayManager::getConnectedRouteObject(std::shared_ptr<MessageServer::Connection> &conn) {
    auto it = _connection2RouteObjectMap.find(conn.get());
    if (it == _connection2RouteObjectMap.end()) {
        return nullptr;
    }

    return it->second;
}

void GatewayManager::addConnectedRouteObject(std::shared_ptr<RouteObject> &ro) {
    _userId2RouteObjectMap.insert(std::make_pair(ro->getUserId(), ro));
    _connection2RouteObjectMap.insert(std::make_pair(ro->getConn().get(), ro));
}

int GatewayManager::tryChangeRouteObjectConn(UserId userId, const std::string &token, std::shared_ptr<MessageServer::Connection> &newConn) {
    // 如果玩家路由对象已存在，将路由对象中的conn更换，并将原conn断线，转移消息缓存
    auto it = _userId2RouteObjectMap.find(userId);
    if (it != _userId2RouteObjectMap.end()) {
        assert(_connection2RouteObjectMap.find(it->second->getConn().get()) != _connection2RouteObjectMap.end());
        if (it->second->getToken() == token) {
            _connection2RouteObjectMap.erase(it->second->getConn().get());

            if (it->second->getConn()->isOpen()) {
                it->second->getConn()->close();
            }

            // 转移消息缓存
            newConn->setMsgBuffer(it->second->getConn()->getMsgBuffer());
            it->second->setConn(newConn);
            _connection2RouteObjectMap.insert(std::make_pair(newConn.get(), it->second));
            return 1;
        } else {
            return -1;
        }
    }

    // 若路由对象处于断线状态中，从断线表移出，并移入正常表，设置路由对象中的conn
    auto it1 = _disconnectedNodeMap.find(userId);
    if (it1 != _disconnectedNodeMap.end()) {
        assert(_userId2RouteObjectMap.find(userId) == _userId2RouteObjectMap.end());

        if (it1->second->data->getToken() == token) {
            // 转移消息缓存
            newConn->setMsgBuffer(it1->second->data->getConn()->getMsgBuffer());
            _userId2RouteObjectMap.insert(std::make_pair(userId, it1->second->data));
            _connection2RouteObjectMap.insert(std::make_pair(newConn.get(), it1->second->data));

            _disconnectedLink.erase(it1->second);
            _disconnectedNodeMap.erase(userId);
            return 1;
        } else {
            return -1;
        }
    }

    return 0;
}

bool GatewayManager::tryMoveToDisconnectedLink(std::shared_ptr<MessageServer::Connection> &conn) {
    auto it = _connection2RouteObjectMap.find(conn.get());
    if (it == _connection2RouteObjectMap.end()) {
        return false;
    }

    auto it1 = _userId2RouteObjectMap.find(it->second->getUserId());
    assert(it1 != _userId2RouteObjectMap.end());
    _userId2RouteObjectMap.erase(it1);

    RouteObjectTimeLink::Node *node = new RouteObjectTimeLink::Node;
    node->data = std::move(it->second);
    _connection2RouteObjectMap.erase(it);
    _disconnectedLink.push(node);
    _disconnectedNodeMap.insert(std::make_pair(node->data->getUserId(), node));
    return true;
}

void *GatewayManager::clearExpiredUnauthRoutine(void *arg) {
    GatewayManager* self = (GatewayManager*)arg;
    
    time_t now;
    
    // 每秒清理一次超时连接
    while (true) {
        sleep(1);
        
        time(&now);
        
        MessageConnectionTimeLink::Node *node = self->_unauthLink.getHead();
        while (node && node->time + g_GatewayConfig.getVerifyTimeout() < now) {
            node->data->close();
            self->removeUnauthConn(node->data);
            node = self->_unauthLink.getHead();
        }
    }
    
    return NULL;
}

void *GatewayManager::clearExpiredDisconnectedRoutine( void *arg ) {
    GatewayManager* self = (GatewayManager*)arg;

    time_t now;
    
    // 每秒清理一次超时断线路由对象
    while (true) {
        sleep(1);
        
        time(&now);
        
        RouteObjectTimeLink::Node *node = self->_disconnectedLink.getHead();
        while (node && node->time + g_GatewayConfig.getDisconnectTimeout() < now) {
            self->removeRouteObject(node->data->getUserId());
            node = self->_disconnectedLink.getHead();
        }
    }
    
    return NULL;
}
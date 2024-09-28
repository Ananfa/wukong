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
#include "gateway_object_manager.h"
#include "gateway_config.h"

#include <sys/time.h>

using namespace wukong;

void GatewayObjectManager::init() {
    RoutineEnvironment::startCoroutine(clearExpiredUnauthRoutine, this);
    RoutineEnvironment::startCoroutine(clearExpiredDisconnectedRoutine, this);
}

void GatewayObjectManager::shutdown() {
    if (shutdown_) {
        return;
    }

    shutdown_ = true;

    // 销毁所有未认证连接
    clearUnauth();

    // 销毁所有网关对象
    clearGatewayObject();
}

void GatewayObjectManager::addUnauthConn(std::shared_ptr<MessageTerminal::Connection>& conn) {
    assert(unauthNodeMap_.find(conn.get()) == unauthNodeMap_.end());
    MessageConnectionTimeLink::Node *node = new MessageConnectionTimeLink::Node;
    node->data = conn;
    unauthLink_.push(node);
    
    unauthNodeMap_.insert(std::make_pair(conn.get(), node));
}

void GatewayObjectManager::removeUnauthConn(std::shared_ptr<MessageTerminal::Connection>& conn) {
    auto kv = unauthNodeMap_.find(conn.get());
    if (kv == unauthNodeMap_.end()) {
        return;
    }
    
    MessageConnectionTimeLink::Node *node = kv->second;
    unauthLink_.erase(node);
    unauthNodeMap_.erase(kv);
}

bool GatewayObjectManager::isUnauth(std::shared_ptr<MessageTerminal::Connection>& conn) {
    return unauthNodeMap_.find(conn.get()) != unauthNodeMap_.end();
}

void GatewayObjectManager::clearUnauth() {
    MessageConnectionTimeLink::Node *node = unauthLink_.getHead();
    while (node) {
        node->data->close();
        node = node->next;
    }
    
    unauthLink_.clear();
    unauthNodeMap_.clear();
}

bool GatewayObjectManager::hasGatewayObject(UserId userId) {
    return userId2GatewayObjectMap_.find(userId) != userId2GatewayObjectMap_.end() || disconnectedNodeMap_.find(userId) != disconnectedNodeMap_.end();
}

bool GatewayObjectManager::removeGatewayObject(UserId userId) {
    auto it = userId2GatewayObjectMap_.find(userId);
    if (it != userId2GatewayObjectMap_.end()) {
        auto obj = it->second;
        auto it1 = connection2GatewayObjectMap_.find(obj->getConn().get());
        assert(it1 != connection2GatewayObjectMap_.end());
        connection2GatewayObjectMap_.erase(it1);
        userId2GatewayObjectMap_.erase(it);
        obj->getConn()->close();
        obj->stop();    // 注意：stop过程会进行协程切换，会产生同步问题
        return true;
    }

    auto it2 = disconnectedNodeMap_.find(userId);
    if (it2 != disconnectedNodeMap_.end()) {
        auto obj = it2->second->data;
        disconnectedLink_.erase(it2->second);
        disconnectedNodeMap_.erase(it2);
        obj->stop();    // 注意：stop过程会进行协程切换，会产生同步问题
        return true;
    }

    return false;
}

size_t GatewayObjectManager::getGatewayObjectNum() {
    return userId2GatewayObjectMap_.size() + disconnectedNodeMap_.size();
}

void GatewayObjectManager::clearGatewayObject() {
    // 销毁所有正常routeObject
    for (auto &pair : userId2GatewayObjectMap_) {
        pair.second->getConn()->close();
        pair.second->stop();
    }
    userId2GatewayObjectMap_.clear();
    connection2GatewayObjectMap_.clear();

    // 销毁所有断线routeObject
    for (auto &pair : disconnectedNodeMap_) {
        pair.second->data->stop();
    }
    disconnectedNodeMap_.clear();
    disconnectedLink_.clear();
}

std::shared_ptr<GatewayObject> GatewayObjectManager::getGatewayObject(UserId userId) {
    auto it = userId2GatewayObjectMap_.find(userId);
    if (it != userId2GatewayObjectMap_.end()) {
        return it->second;
    }

    auto it2 = disconnectedNodeMap_.find(userId);
    if (it2 != disconnectedNodeMap_.end()) {
        return it2->second->data;
    }

    return nullptr;
}

std::shared_ptr<GatewayObject> GatewayObjectManager::getConnectedGatewayObject(UserId userId) {
    auto it = userId2GatewayObjectMap_.find(userId);
    if (it == userId2GatewayObjectMap_.end()) {
        return nullptr;
    }

    return it->second;
}

std::shared_ptr<GatewayObject> GatewayObjectManager::getConnectedGatewayObject(std::shared_ptr<MessageTerminal::Connection> &conn) {
    auto it = connection2GatewayObjectMap_.find(conn.get());
    if (it == connection2GatewayObjectMap_.end()) {
        return nullptr;
    }

    return it->second;
}

void GatewayObjectManager::traverseConnectedGatewayObject(std::function<bool(std::shared_ptr<GatewayObject>&)> handle) {
    for (auto &pair : userId2GatewayObjectMap_) {
        if (!handle(pair.second)) {
            return;
        }
    }
}

void GatewayObjectManager::addConnectedGatewayObject(std::shared_ptr<GatewayObject> &obj) {
    userId2GatewayObjectMap_.insert(std::make_pair(obj->getUserId(), obj));
    connection2GatewayObjectMap_.insert(std::make_pair(obj->getConn().get(), obj));
}

int GatewayObjectManager::tryChangeGatewayObjectConn(UserId userId, const std::string &token, std::shared_ptr<MessageTerminal::Connection> &newConn) {
    // 如果玩家网关对象已存在，将网关对象中的conn更换，并将原conn断线，转移消息缓存
    auto it = userId2GatewayObjectMap_.find(userId);
    if (it != userId2GatewayObjectMap_.end()) {
        assert(connection2GatewayObjectMap_.find(it->second->getConn().get()) != connection2GatewayObjectMap_.end());
        if (it->second->getGToken() == token) {
            connection2GatewayObjectMap_.erase(it->second->getConn().get());

            if (it->second->getConn()->isOpen()) {
                it->second->getConn()->close();
            }

            // 转移消息缓存
            newConn->setLastSendSerial(it->second->getConn()->getLastSendSerial());
            newConn->setMsgBuffer(it->second->getConn()->getMsgBuffer());
            it->second->setConn(newConn);
            connection2GatewayObjectMap_.insert(std::make_pair(newConn.get(), it->second));
            return 1;
        } else {
            return -1;
        }
    }

    // 若网关对象处于断线状态中，从断线表移出，并移入正常表，设置网关对象中的conn
    auto it1 = disconnectedNodeMap_.find(userId);
    if (it1 != disconnectedNodeMap_.end()) {
        assert(userId2GatewayObjectMap_.find(userId) == userId2GatewayObjectMap_.end());

        if (it1->second->data->getGToken() == token) {
            // 转移消息缓存
            newConn->setLastSendSerial(it1->second->data->getConn()->getLastSendSerial());
            newConn->setMsgBuffer(it1->second->data->getConn()->getMsgBuffer());
            it1->second->data->setConn(newConn);
            userId2GatewayObjectMap_.insert(std::make_pair(userId, it1->second->data));
            connection2GatewayObjectMap_.insert(std::make_pair(newConn.get(), it1->second->data));

            disconnectedLink_.erase(it1->second);
            disconnectedNodeMap_.erase(userId);
            return 1;
        } else {
            return -1;
        }
    }

    // 设置新连接的消息缓存
    std::shared_ptr<MessageBuffer> msgBuf = std::shared_ptr<MessageBuffer>(new MessageBuffer(g_GatewayConfig.getMaxBufMsgNum()));
    newConn->setMsgBuffer(msgBuf);

    return 0;
}

bool GatewayObjectManager::tryMoveToDisconnectedLink(std::shared_ptr<MessageTerminal::Connection> &conn) {
    auto it = connection2GatewayObjectMap_.find(conn.get());
    if (it == connection2GatewayObjectMap_.end()) {
        return false;
    }

    auto it1 = userId2GatewayObjectMap_.find(it->second->getUserId());
    assert(it1 != userId2GatewayObjectMap_.end());
    userId2GatewayObjectMap_.erase(it1);

    GatewayObjectTimeLink::Node *node = new GatewayObjectTimeLink::Node;
    node->data = std::move(it->second);
    connection2GatewayObjectMap_.erase(it);
    disconnectedLink_.push(node);
    disconnectedNodeMap_.insert(std::make_pair(node->data->getUserId(), node));
    return true;
}

void *GatewayObjectManager::clearExpiredUnauthRoutine(void *arg) {
    GatewayObjectManager* self = (GatewayObjectManager*)arg;
    
    time_t now;
    
    // 每秒清理一次超时连接
    while (true) {
        sleep(1);
        
        time(&now);
        
        MessageConnectionTimeLink::Node *node = self->unauthLink_.getHead();
        while (node && node->time + g_GatewayConfig.getVerifyTimeout() < now) {
            node->data->close();
            self->removeUnauthConn(node->data);
            node = self->unauthLink_.getHead();
        }
    }
    
    return NULL;
}

void *GatewayObjectManager::clearExpiredDisconnectedRoutine( void *arg ) {
    GatewayObjectManager* self = (GatewayObjectManager*)arg;

    time_t now;
    
    // 每秒清理一次超时断线网关对象
    while (true) {
        sleep(1);
        
        time(&now);
        
        GatewayObjectTimeLink::Node *node = self->disconnectedLink_.getHead();
        while (node && node->time + g_GatewayConfig.getDisconnectTimeout() < now) {
            self->removeGatewayObject(node->data->getUserId());
            node = self->disconnectedLink_.getHead();
        }
    }
    
    return NULL;
}
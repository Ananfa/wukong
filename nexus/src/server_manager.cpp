/*
 * Created by Xianke Liu on 2024/7/6.
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

#include "server_manager.h"

#include "corpc_routine_env.h"
#include "nexus_config.h"
#include "share/const.h"

using namespace wukong;

bool ServerManager::start() {
    if (started) {
        return false;
    }

    started = true;
    RoutineEnvironment::startCoroutine(clearExpiredUnaccessRoutine, this);
    RoutineEnvironment::startCoroutine(clearExpiredDisconnectedRoutine, this);
    return true;
}

void ServerManager::addUnaccessConn(std::shared_ptr<MessageTerminal::Connection>& conn) {
    assert(unaccessNodeMap_.find(conn.get()) == unaccessNodeMap_.end());
    MessageConnectionTimeLink::Node *node = new MessageConnectionTimeLink::Node;
    node->data = conn;
    unaccessLink_.push(node);
    
    unaccessNodeMap_.insert(std::make_pair(conn.get(), node));
}

void ServerManager::removeUnaccessConn(std::shared_ptr<MessageTerminal::Connection>& conn) {
    auto kv = unaccessNodeMap_.find(conn.get());
    if (kv == unaccessNodeMap_.end()) {
        return;
    }
    
    MessageConnectionTimeLink::Node *node = kv->second;
    unaccessLink_.erase(node);
    unaccessNodeMap_.erase(kv);
}

bool ServerManager::isUnaccess(std::shared_ptr<MessageTerminal::Connection>& conn) {
    return unaccessNodeMap_.find(conn.get()) != unaccessNodeMap_.end();
}

void ServerManager::clearUnaccess() {
    MessageConnectionTimeLink::Node *node = unaccessLink_.getHead();
    while (node) {
        node->data->close();
        node = node->next;
    }
    
    unaccessLink_.clear();
    unaccessNodeMap_.clear();
}

bool ServerManager::hasServerObject(ServerType stype, ServerId sid) {
    auto it1 = serverObjectMap_.find(stype);
    if (it1 != serverObjectMap_.end()) {
        auto it2 = it1->second.find(sid);
        if (it2 != it1->second.end()) {
            return true;
        }
    }

    auto it3 = disconnectedNodeMap_.find(stype);
    if (it3 != disconnectedNodeMap_.end()) {
        auto it4 = it3->second.find(sid);
        if (it4 != it3->second.end()) {
            return true;
        }
    }

    return false;
}

bool ServerManager::removeServerObject(ServerType stype, ServerId sid) {
    auto it1 = serverObjectMap_.find(stype);
    if (it1 != serverObjectMap_.end()) {
        auto it2 = it1->second.find(sid);
        if (it2 != it1->second.end()) {
            auto obj = it2->second;
            auto it3 = connection2ServerObjectMap_.find(obj->getConn().get());
            assert(it3 != connection2ServerObjectMap_.end());
            connection2ServerObjectMap_.erase(it3);
            it1->second.erase(it2);
            if (it1->second.empty()) {
                serverObjectMap_.erase(it1);
            }
            obj->getConn()->close();
            return true;
        }
    }

    auto it4 = disconnectedNodeMap_.find(stype);
    if (it4 != disconnectedNodeMap_.end()) {
        auto it5 = it4->second.find(sid);
        if (it5 != it4->second.end()) {
            disconnectedLink_.erase(it5->second);
            it4->second.erase(it5);
            if (it4->second.empty()) {
                disconnectedNodeMap_.erase(it4);
            }
            return true;
        }
    }

    return false;
}

//bool ServerManager::isDisconnectedObject(ServerType stype, ServerId sid) {
//    auto it1 = disconnectedNodeMap_.find(stype);
//    if (it1 != disconnectedNodeMap_.end()) {
//        auto it2 = it1->second.find(sid);
//        if (it2 != it1->second.end()) {
//            return true;
//        }
//    }
//
//    return false;
//}

std::shared_ptr<ServerObject> ServerManager::getServerObject(ServerType stype, ServerId sid) {
    auto it1 = serverObjectMap_.find(stype);
    if (it1 != serverObjectMap_.end()) {
        auto it2 = it1->second.find(sid);
        if (it2 != it1->second.end()) {
            return it2->second;
        }
    }

    auto it3 = disconnectedNodeMap_.find(stype);
    if (it3 != disconnectedNodeMap_.end()) {
        auto it4 = it3->second.find(sid);
        if (it4 != it3->second.end()) {
            return it4->second->data;
        }
    }

    return nullptr;
}

std::shared_ptr<ServerObject> ServerManager::getConnectedServerObject(std::shared_ptr<MessageTerminal::Connection> &conn) {
    auto it = connection2ServerObjectMap_.find(conn.get());
    if (it == connection2ServerObjectMap_.end()) {
        return nullptr;
    }

    return it->second;
}

const std::map<ServerId, std::shared_ptr<ServerObject>>& ServerManager::getServerObjects(ServerType stype) {
    auto it = serverObjectMap_.find(stype);
    if (it == serverObjectMap_.end()) {
        return emptyServerObjectMap_;
    }

    return it->second;
}

void ServerManager::moveToDisconnected(std::shared_ptr<ServerObject> &obj) {
    ServerType stype = obj->getType();
    ServerId sid = obj->getId();

    auto it1 = serverObjectMap_.find(stype);
    assert(it1 != serverObjectMap_.end());
    auto it2 = it1->second.find(sid);
    assert(it2 != it1->second.end());

    it1->second.erase(it2);
    if (it1->second.empty()) {
        serverObjectMap_.erase(it1);
    }

    connection2ServerObjectMap_.erase(obj->getConn().get());
    obj->resetConn();

    ServerObjectTimeLink::Node *node = new ServerObjectTimeLink::Node;
    node->data = obj;
    disconnectedLink_.push(node);
    disconnectedNodeMap_[stype].insert(std::make_pair(sid, node));
}

void ServerManager::addConnectedServer(std::shared_ptr<ServerObject> &obj) {
    ServerType stype = obj->getType();
    ServerId sid = obj->getId();

    auto it1 = disconnectedNodeMap_.find(stype);
    if (it1 != disconnectedNodeMap_.end()) {
        auto it2 = it1->second.find(sid);
        if (it2 != it1->second.end()) {
            assert(it2->second->data == obj);
            
            disconnectedLink_.erase(it2->second);
            it1->second.erase(it2);
            if (it1->second.empty()) {
                disconnectedNodeMap_.erase(it1);
            }
        }
    }
    
    serverObjectMap_[stype].insert(std::make_pair(sid, obj));
    connection2ServerObjectMap_.insert(std::make_pair(obj->getConn().get(), obj));
}

void *ServerManager::clearExpiredUnaccessRoutine(void *arg) {
    ServerManager* self = (ServerManager*)arg;
    
    time_t now;
    
    // 每秒清理一次超时连接
    while (true) {
        sleep(1);
        
        time(&now);
        
        MessageConnectionTimeLink::Node *node = self->unaccessLink_.getHead();
        while (node && node->time + g_NexusConfig.getAccessTimeout() < now) {
            node->data->close();
            self->removeUnaccessConn(node->data);
            node = self->unaccessLink_.getHead();
        }
    }
    
    return NULL;
}

void *ServerManager::clearExpiredDisconnectedRoutine( void *arg ) {
    ServerManager* self = (ServerManager*)arg;

    time_t now;
    
    // 每秒清理一次超时断线服务对象
    while (true) {
        sleep(1);
        
        time(&now);
        
        ServerObjectTimeLink::Node *node = self->disconnectedLink_.getHead();
        while (node && node->time + g_NexusConfig.getDisconnectTimeout() < now) {
            self->removeServerObject(node->data->getType(), node->data->getId());

            // 向关注新接入服务器的服务器发移除服务器信息
            auto& beConcerns = g_NexusConfig.getBeConcerns(node->data->getType());
            if (!beConcerns.empty()) {
                std::shared_ptr<pb::RemoveServerNtf> ntf(new pb::RemoveServerNtf);
                ntf->set_server_type(node->data->getType());
                ntf->set_server_id(node->data->getId());

                for (auto beConcernType : beConcerns) {
                    auto &objMap = self->getServerObjects(beConcernType);
                    for (auto it : objMap) {
                        it.second->send(N2S_MESSAGE_ID_RMSVR, ntf);
                    }
                }
            }

            node = self->disconnectedLink_.getHead();
        }
    }
    
    return NULL;
}
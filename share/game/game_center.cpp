/*
 * Created by Xianke Liu on 2021/1/15.
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

#include "game_center.h"
#include "redis_pool.h"
#include "share/const.h"
#include "corpc_pubsub.h"

using namespace wukong;

void GameCenter::init(GameServerType stype, uint32_t gameObjectUpdatePeriod) {
    _type = stype;
    _gameObjectUpdatePeriod = gameObjectUpdatePeriod;

    RoutineEnvironment::startCoroutine(registerEventQueueRoutine, this); // 注册事件通知队列

    // 初始化全服事件发布订阅服务
    // 获取并订阅服务器组列表信息
    std::list<std::string> topics;
    topics.push_back("GEvent");
    PubsubService::StartPubsubService(g_RedisPoolManager.getCoreCache()->getPool(), topics);
    PubsubService::Subscribe("GEvent", false, std::bind(&GameCenter::handleGlobalEvent, this, std::placeholders::_1, std::placeholders::_2));
}

bool GameCenter::registerMessage(int msgType,
                                    google::protobuf::Message *proto,
                                    bool needCoroutine,
                                    MessageHandle handle) {
    if (_registerMessageMap.find(msgType) != _registerMessageMap.end()) {
        return false;
    }

    RegisterMessageInfo info;
    info.proto = proto;
    info.needCoroutine = needCoroutine;
    info.handle = handle;
    
    _registerMessageMap.insert(std::make_pair(msgType, info));
    
    return true;
}

void GameCenter::handleMessage(std::shared_ptr<GameObject> obj, int msgType, uint16_t tag, const std::string &rawMsg) {
    auto iter = _registerMessageMap.find(msgType);
    if (iter == _registerMessageMap.end()) {
        ERROR_LOG("GameCenter::handleMessage -- unknown message type: %d\n", msgType);
        return;
    }

    google::protobuf::Message *msg = nullptr;
    if (iter->second.proto) {
        msg = iter->second.proto->New();
        if (!msg->ParseFromString(rawMsg)) {
            // 出错处理
            ERROR_LOG("GameCenter::handleMessage -- parse fail for message: %d\n", msgType);
            delete msg;
            return;
        } else {
            assert(!rawMsg.empty());
        }
    }

    std::shared_ptr<google::protobuf::Message> targetMsg = std::shared_ptr<google::protobuf::Message>(msg);

    if (iter->second.needCoroutine) {
        HandleMessageInfo *info = new HandleMessageInfo();
        info->obj = obj;
        info->msg = targetMsg;
        info->tag = tag;
        info->handle = iter->second.handle;

        RoutineEnvironment::startCoroutine(handleMessageRoutine, info);
        return;
    }

    iter->second.handle(obj, tag, targetMsg);
}

void *GameCenter::handleMessageRoutine(void *arg) {
    HandleMessageInfo *info = (HandleMessageInfo *)arg;

    info->handle(info->obj, info->tag, info->msg);
    delete info;
    
    return nullptr;
}

void *GameCenter::registerEventQueueRoutine(void * arg) {
    GameCenter *self = (GameCenter *)arg;

    GlobalEventRegisterQueue& queue = self->_eventRegisterQueue;

    // 初始化pipe readfd
    int readFd = queue.getReadFd();
    co_register_fd(readFd);
    co_set_timeout(readFd, -1, 1000);
    
    int ret;
    std::vector<char> buf(1024);
    while (true) {
        // 等待处理信号
        ret = (int)read(readFd, &buf[0], 1024);
        assert(ret != 0);
        if (ret < 0) {
            if (errno == EAGAIN) {
                continue;
            } else {
                // 管道出错
                ERROR_LOG("GameCenter::registerEventQueueRoutine read from pipe fd %d ret %d errno %d (%s)\n",
                       readFd, ret, errno, strerror(errno));
                
                // TODO: 如何处理？退出协程？
                // sleep 10 milisecond
                msleep(10);
            }
        }
        
        // 处理任务队列
        std::shared_ptr<GlobalEventQueue> eventQueue = queue.pop();
        while (eventQueue) {
            self->_eventQueues.push_back(eventQueue);
                        
            eventQueue = queue.pop();
        }
    }
    
    return NULL;
}

void GameCenter::registerEventQueue(std::shared_ptr<GlobalEventQueue> queue) {
    _eventRegisterQueue.push(queue);
}

void GameCenter::handleGlobalEvent(const std::string& topic, const std::string& msg) {
    // 解析消息
    std::shared_ptr<wukong::pb::GlobalEventMessage> eventMsg = std::make_shared<wukong::pb::GlobalEventMessage>();
    if (!eventMsg->ParseFromString(msg)) {
        ERROR_LOG("GameCenter::handleGlobalEvent -- parse event data failed\n");
        return;
    }

    for (auto queue : _eventQueues) {
        queue->push(eventMsg);
    }
}

/*
 * Created by Xianke Liu on 2022/5/11.
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

#include "global_event.h"
#include "redis_pool.h"
#include "corpc_pubsub.h"

using namespace wukong;

void GlobalEventListener::init() {
    RoutineEnvironment::startCoroutine(registerEventQueueRoutine, this); // 注册事件通知队列

    // 初始化全服事件发布订阅服务
    // 获取并订阅服务器组列表信息
    std::list<std::string> topics;
    topics.push_back("GEvent");
    PubsubService::StartPubsubService(g_RedisPoolManager.getCoreCache()->getPool(), topics);
    PubsubService::Subscribe("GEvent", false, std::bind(&GlobalEventListener::handleGlobalEvent, this, std::placeholders::_1, std::placeholders::_2));
}

void GlobalEventListener::registerEventQueue(std::shared_ptr<GlobalEventQueue> queue) {
    _eventRegisterQueue.push(queue);
}

void *GlobalEventListener::registerEventQueueRoutine(void * arg) {
    GlobalEventListener *self = (GlobalEventListener *)arg;

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
                ERROR_LOG("GlobalEventListener::registerEventQueueRoutine read from pipe fd %d ret %d errno %d (%s)\n",
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

void GlobalEventListener::handleGlobalEvent(const std::string& topic, const std::string& msg) {
    // 解析消息
    std::shared_ptr<wukong::pb::GlobalEventMessage> eventMsg = std::make_shared<wukong::pb::GlobalEventMessage>();
    if (!eventMsg->ParseFromString(msg)) {
        ERROR_LOG("GlobalEventListener::handleGlobalEvent -- parse event data failed\n");
        return;
    }

    for (auto queue : _eventQueues) {
        queue->push(eventMsg);
    }
}

void GlobalEventDispatcher::init(GlobalEventListener &listener) {
    _eventQueue = std::make_shared<GlobalEventQueue>();

    RoutineEnvironment::startCoroutine(globalEventHandleRoutine, this);

    listener.registerEventQueue(_eventQueue);
}

uint32_t GlobalEventDispatcher::regGlobalEventHandle(const std::string &name, EventHandle handle) {
    return _emiter.addEventHandle(name, handle);
}

void GlobalEventDispatcher::unregGlobalEventHandle(uint32_t refId) {
    _emiter.removeEventHandle(refId);
}

void GlobalEventDispatcher::fireGlobalEvent(const Event &event) {
    std::string data;
    if (!event.getParam("data", data)) {
        ERROR_LOG("GameObjectManager::fireGlobalEvent -- event hasn't data param\n");
    }

    wukong::pb::GlobalEventMessage message;
    message.set_topic(event.getName());
    message.set_data(data);

    std::string pData;
    message.SerializeToString(&pData);

    PubsubService::Publish("GEvent", pData);
}

void *GlobalEventDispatcher::globalEventHandleRoutine(void * arg) {
    GlobalEventDispatcher *self = (GlobalEventDispatcher *)arg;

    // 初始化pipe readfd
    int readFd = self->_eventQueue->getReadFd();
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
                ERROR_LOG("GlobalEventDispatcher::globalEventHandleRoutine -- read from pipe fd %d ret %d errno %d (%s)\n",
                       readFd, ret, errno, strerror(errno));
                
                // TODO: 如何处理？退出协程？
                // sleep 10 milisecond
                msleep(10);
            }
        }
        
        // 分派事件处理
        std::shared_ptr<wukong::pb::GlobalEventMessage> message = self->_eventQueue->pop();
        while (message) {
            Event e(message->topic().c_str());
            e.setParam("data", message->data());
            self->_emiter.fireEvent(e);
            
            message = self->_eventQueue->pop();
        }
    }
    
    return NULL;
}

void GlobalEventDispatcher::clear() {
    _emiter.clear();
}
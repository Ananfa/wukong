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

#ifndef global_event_h
#define global_event_h

#include "share/define.h"
#include "event.h"

namespace wukong {
    // 注意：此全局事件相关的基础设施（包括GlobalEventListener和GlobalEventDispatcher都不对销毁进行处理，服务器启动时一次性初始化完成，跟随进程一起销毁）
    // 注意：GlobalEventListener中绑定了发布订阅服务（而且只订阅GEvent一个主题）
    // GlobalEventListener通过Redis的发布订阅接口来监听全服事件，并通知GlobalEventDispatcher分发事件
    // GlobalEventListener与GlobalEventDispatcher可在不同的线程中
    class GlobalEventListener {
    public:
        GlobalEventListener() {}
        ~GlobalEventListener() {}

        void init();
    private:
        std::list<std::shared_ptr<GlobalEventQueue>> _eventQueues; // 全服事件通知队列列表

        GlobalEventRegisterQueue _eventRegisterQueue; // 用于多线程同步注册事件队列时

    private:
        void registerEventQueue(std::shared_ptr<GlobalEventQueue> queue);
        static void *registerEventQueueRoutine(void * arg);

        void handleGlobalEvent(const std::string& topic, const std::string& msg);
    public:
        friend class GlobalEventDispatcher;

    private:
        // 禁止在堆中创建对象
        void* operator new(size_t t) {}
        void operator delete(void* ptr) {}

        // 禁止拷贝
        GlobalEventListener(GlobalEventListener const&) = delete;                    // copy ctor delete
        GlobalEventListener(GlobalEventListener &&) = delete;                        // move ctor delete
        GlobalEventListener& operator=(GlobalEventListener const&) = delete;         // assign op. delete
        GlobalEventListener& operator=(GlobalEventListener &&) = delete;             // move assign op. delete
    };

    // GlobalEventDispatcher分发全服事件
    class GlobalEventDispatcher {
    public:
        GlobalEventDispatcher() {}
        ~GlobalEventDispatcher() {}

        void init(GlobalEventListener &listener);

        // 全服事件相关接口
        uint32_t regGlobalEventHandle(const std::string &name, EventHandle handle);
        void unregGlobalEventHandle(uint32_t refId);

        void fireGlobalEvent(const Event &event); // 注意：全局事件中只能放一个叫"data"的std::string类型数据

        void clear();

    private:
        // 全服事件相关
        static void *globalEventHandleRoutine(void * arg);

    private:
        EventEmitter _emiter;
        std::shared_ptr<GlobalEventQueue> _eventQueue;

    public:
        friend class GlobalEventListener;
    };
}

#endif /* global_event_h */

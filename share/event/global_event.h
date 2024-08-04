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

#ifndef wukong_global_event_h
#define wukong_global_event_h

#include "share/define.h"
#include "event.h"

namespace wukong {
    class GlobalEventEmitter {
    public:
        static GlobalEventEmitter& Instance() {
            static GlobalEventEmitter instance;
            return instance;
        }

        void init();

        void regEventHandle(const std::string &name, EventHandle handle);

        void fireEvent(const Event &event); // 注意：全局事件中只能放一个叫"data"的std::string类型数据

        void clear();

    private:
        EventEmitter emiter_;

    private:
        GlobalEventEmitter() {}                                                    // ctor hidden
        ~GlobalEventEmitter() = default;                                           // destruct hidden
        GlobalEventEmitter(GlobalEventEmitter const&) = delete;                    // copy ctor delete
        GlobalEventEmitter(GlobalEventEmitter &&) = delete;                        // move ctor delete
        GlobalEventEmitter& operator=(GlobalEventEmitter const&) = delete;         // assign op. delete
        GlobalEventEmitter& operator=(GlobalEventEmitter &&) = delete;             // move assign op. delete
    };
}

#define g_GlobalEventEmitter wukong::GlobalEventEmitter::Instance()

#endif /* wukong_global_event_h */

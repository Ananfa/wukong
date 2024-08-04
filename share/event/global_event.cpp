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

void GlobalEventEmitter::init() {
    PubsubService::Subscribe("WK_GEvent", false, [this](const std::string& topic, const std::string& msg) {
        // 解析消息
        wukong::pb::GlobalEventMessage eventMsg;
        if (!eventMsg.ParseFromString(msg)) {
            ERROR_LOG("GEvent message handle -- parse event data failed\n");
            return;
        }

        Event e(eventMsg.topic().c_str());
        e.setParam("data", eventMsg.data());
        emiter_.fireEvent(e);
    });
}

void GlobalEventEmitter::regEventHandle(const std::string &name, EventHandle handle) {
    emiter_.regEventHandle(name, handle);
}

void GlobalEventEmitter::fireEvent(const Event &event) {
    std::string data;
    if (!event.getParam("data", data)) {
        ERROR_LOG("GlobalEventEmitter::fireGlobalEvent -- event hasn't data param\n");
    }

    wukong::pb::GlobalEventMessage message;
    message.set_topic(event.getName());
    message.set_data(data);

    std::string pData;
    message.SerializeToString(&pData);

    PubsubService::Publish("WK_GEvent", pData);
}

void GlobalEventEmitter::clear() {
    emiter_.clear();
}

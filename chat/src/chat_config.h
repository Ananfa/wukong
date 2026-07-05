/*
 * Created by Xianke Liu on 2025/5/21.
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

#ifndef wukong_chat_config_h
#define wukong_chat_config_h

#include <stdio.h>
#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class ChatConfig {
    public:
        static ChatConfig& Instance() {
            static ChatConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getIp() const { return ip_; }
        uint16_t getPort() const { return port_; }

        uint32_t getIoRecvThreadNum() const { return ioRecvThreadNum_; }
        uint32_t getIoSendThreadNum() const { return ioSendThreadNum_; }

        const Address& getNexusAddr() const { return nexusAddr_; }
        
    private:
        std::string ip_;    // 提供rpc服务的ip
        uint16_t port_;     // rpc服务端口

        uint32_t ioRecvThreadNum_;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t ioSendThreadNum_;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        Address nexusAddr_;     // nexus服务地址
        
    private:
        ChatConfig() = default;                               // ctor hidden
        ChatConfig(ChatConfig const&) = delete;               // copy ctor hidden
        ChatConfig(ChatConfig &&) = delete;                   // move ctor hidden
        ChatConfig& operator=(ChatConfig const&) = delete;    // assign op. hidden
        ChatConfig& operator=(ChatConfig &&) = delete;        // move assign op. hidden
        ~ChatConfig() = default;                              // dtor hidden
    };

}

#define g_ChatConfig wukong::ChatConfig::Instance()

#endif /* wukong_chat_config_h */

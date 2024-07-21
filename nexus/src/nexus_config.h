/*
 * Created by Xianke Liu on 2024/7/5.
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

#ifndef wukong_nexus_config_h
#define wukong_nexus_config_h

#include <stdio.h>
#include <string>
#include <map>
#include <set>

namespace wukong {

    // 单例模式实现
    class NexusConfig {
    //public:
    //    struct ConcernInfo {
    //        int32_t server_type_;
    //        std::set<int32_t> concern_server_types_;
    //    };

    public:
        static NexusConfig& Instance() {
            static NexusConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getIp() const { return ip_; }
        uint16_t getPort() const { return port_; }

        uint32_t getAccessTimeout() const { return accessTimeout_; }
        uint32_t getDisconnectTimeout() const { return disconnectTimeout_; }
        uint32_t getIoRecvThreadNum() const { return ioRecvThreadNum_; }
        uint32_t getIoSendThreadNum() const { return ioSendThreadNum_; }

        const std::set<uint32_t> &getConcerns(uint32_t server_type);
        const std::set<uint32_t> &getBeConcerns(uint32_t server_type);

    private:
        std::string ip_;    // 提供rpc服务的ip
        uint16_t port_;     // rpc服务端口

        uint32_t accessTimeout_;        // 接入超时（秒）
        uint32_t disconnectTimeout_;    // 断线保护时长（秒）
        uint32_t ioRecvThreadNum_;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t ioSendThreadNum_;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        std::map<uint32_t, std::set<uint32_t>> concern_map_; // 关注表
        std::map<uint32_t, std::set<uint32_t>> be_concern_map_; // 被关注表
        std::set<uint32_t> empty_concern_set_; // 用于无关注信息时返回

    private:
        NexusConfig() = default;                                // ctor hidden
        NexusConfig(NexusConfig const&) = delete;               // copy ctor hidden
        NexusConfig(NexusConfig &&) = delete;                   // move ctor hidden
        NexusConfig& operator=(NexusConfig const&) = delete;    // assign op. hidden
        NexusConfig& operator=(NexusConfig &&) = delete;        // move assign op. hidden
        ~NexusConfig() = default;                               // dtor hidden
    };

    #define g_NexusConfig NexusConfig::Instance()

}

#endif /* wukong_nexus_config_h */

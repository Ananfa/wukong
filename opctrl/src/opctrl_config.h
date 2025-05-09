/*
 * Created by Xianke Liu on 2025/5/7.
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

#ifndef wukong_opctrl_config_h
#define wukong_opctrl_config_h

#include <stdio.h>
#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class OpctrlConfig {
    public:
        static OpctrlConfig& Instance() {
            static OpctrlConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        uint32_t getId() const { return id_; }

        const std::string& getServiceIp() const { return serviceIp_; }
        const uint16_t getServicePort() const { return servicePort_; }

        uint32_t getIoRecvThreadNum() const { return ioRecvThreadNum_; }
        uint32_t getIoSendThreadNum() const { return ioSendThreadNum_; }

        const std::vector<RedisInfo>& getRedisInfos() const { return redisInfos_; }

        const std::string& getCoreCache() const { return coreCache_; }
        
        const Address& getNexusAddr() const { return nexusAddr_; }
        
    private:
        uint32_t id_;                // 服务号（Gateway服务唯一标识）

        std::string serviceIp_;    // 对外提供http服务的ip
        uint16_t servicePort_;     // 对外提供http服务的端口

        uint32_t ioRecvThreadNum_;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t ioSendThreadNum_;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        std::vector<RedisInfo> redisInfos_; // Redis库配置

        std::string coreCache_;  // 用作游戏服务器核心缓存redis库(redis中的一个)

        Address nexusAddr_;     // nexus服务地址

    private:
        OpctrlConfig() = default;                                 // ctor hidden
        OpctrlConfig(OpctrlConfig const&) = delete;               // copy ctor hidden
        OpctrlConfig(OpctrlConfig &&) = delete;                   // move ctor hidden
        OpctrlConfig& operator=(OpctrlConfig const&) = delete;    // assign op. hidden
        OpctrlConfig& operator=(OpctrlConfig &&) = delete;        // move assign op. hidden
        ~OpctrlConfig() = default;                                // dtor hidden
    };

}

#define g_OpctrlConfig wukong::OpctrlConfig::Instance()

#endif /* wukong_opctrl_config_h */

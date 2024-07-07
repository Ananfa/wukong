/*
 * Created by Xianke Liu on 2024/6/15.
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

#ifndef wukong_front_config_h
#define wukong_front_config_h

#include <stdio.h>
#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class FrontConfig {
    public:
        static FrontConfig& Instance() {
            static FrontConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getIp() const { return ip_; }
        uint16_t getPort() const { return port_; }

        uint32_t getInflowThreadNum() const { return inflowThreadNum_; }
        uint32_t getOutflowThreadNum() const { return outflowThreadNum_; }

        const std::vector<RedisInfo>& getRedisInfos() const { return redisInfos_; }
        
        const std::string& getCoreCache() const { return coreCache_; }

    private:
        std::string ip_;    // 提供rpc服务的ip
        uint16_t port_;     // rpc服务端口

        uint32_t inflowThreadNum_; // 入流线程数量
        uint32_t outflowThreadNum_; // 出流线程数量

        std::vector<RedisInfo> redisInfos_; // Redis库配置

        std::string coreCache_;  // 用作游戏服务器核心缓存redis库(redis中的一个)

    private:
        FrontConfig() = default;                                // ctor hidden
        FrontConfig(FrontConfig const&) = delete;               // copy ctor hidden
        FrontConfig(FrontConfig &&) = delete;                   // move ctor hidden
        FrontConfig& operator=(FrontConfig const&) = delete;    // assign op. hidden
        FrontConfig& operator=(FrontConfig &&) = delete;        // move assign op. hidden
        ~FrontConfig() = default;                               // dtor hidden
    };

    #define g_FrontConfig FrontConfig::Instance()

}

#endif /* wukong_front_config_h */

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

#ifndef wukong_lobby_config_h
#define wukong_lobby_config_h

#include <stdio.h>
#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class LobbyConfig {
    public:
        static LobbyConfig& Instance() {
            static LobbyConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        uint32_t getId() const { return id_; }
        const std::string& getIp() const { return ip_; }
        uint16_t getPort() const { return port_; }

        uint32_t getIoRecvThreadNum() const { return ioRecvThreadNum_; }
        uint32_t getIoSendThreadNum() const { return ioSendThreadNum_; }

        const std::vector<RedisInfo>& getRedisInfos() const { return redisInfos_; }
        
        const std::string& getCoreCache() const { return _coreCache; }
        
        uint32_t getUpdatePeriod() const { return updatePeriod_; }
        
        const std::string& getLogConfigFile() const { return logConfigFile_; }

        const Address& getNexusAddr() const { return nexusAddr_; }
        
    private:
        uint32_t id_;       // 服务号

        std::string ip_;    // 提供rpc服务的ip
        uint16_t port_;     // rpc服务端口

        uint32_t ioRecvThreadNum_;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t ioSendThreadNum_;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        std::vector<RedisInfo> redisInfos_; // Redis库配置

        std::string _coreCache;  // 用作游戏服务器核心缓存redis库(redis中的一个)

        uint32_t updatePeriod_; // 游戏对象update方法调用周期，单位毫秒，0表示不进行update

        std::string logConfigFile_; // 日志配置文件

        Address nexusAddr_;     // nexus服务地址
        
    private:
        LobbyConfig() = default;                                // ctor hidden
        LobbyConfig(LobbyConfig const&) = delete;               // copy ctor hidden
        LobbyConfig(LobbyConfig &&) = delete;                   // move ctor hidden
        LobbyConfig& operator=(LobbyConfig const&) = delete;    // assign op. hidden
        LobbyConfig& operator=(LobbyConfig &&) = delete;        // move assign op. hidden
        ~LobbyConfig() = default;                               // dtor hidden
    };

}

#define g_LobbyConfig wukong::LobbyConfig::Instance()

#endif /* wukong_lobby_config_h */

/*
 * Created by Xianke Liu on 2022/1/12.
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

#ifndef wukong_scene_config_h
#define wukong_scene_config_h

#include <stdio.h>
#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class SceneConfig {
    public:
        struct ServerInfo {
            uint32_t id;            // 服务号（Gateway服务唯一标识，与zookeeper注册发现有关）
        };

    public:
        static SceneConfig& Instance() {
            static SceneConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getIp() const { return ip_; }
        uint16_t getPort() const { return port_; }
        const std::vector<ServerInfo>& getServerInfos() const { return serverInfos_; }
        
        const std::string& getZookeeper() const { return zookeeper_; }
        
        uint32_t getIoRecvThreadNum() const { return ioRecvThreadNum_; }
        uint32_t getIoSendThreadNum() const { return ioSendThreadNum_; }

        const std::vector<RedisInfo>& getRedisInfos() const { return redisInfos_; }

        const std::string& getCoreCache() const { return coreCache_; }
        
        uint32_t getUpdatePeriod() const { return updatePeriod_; }
        
        const std::string& getZooPath() const { return zooPath_; }

        bool enableLobbyClient() const { return enableLobbyClient_; }
        bool enableSceneClient() const { return enableSceneClient_; }

    private:
        std::string ip_;    // 提供rpc服务的ip
        uint16_t port_;     // rpc服务端口

        uint32_t type_;     // 场景服类型

        std::vector<ServerInfo> serverInfos_; // 对外服务的信息列表
        
        std::string zookeeper_;
        
        uint32_t ioRecvThreadNum_;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t ioSendThreadNum_;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        std::vector<RedisInfo> redisInfos_; // Redis库配置

        std::string coreCache_;  // 用作游戏服务器核心缓存redis库(redis中的一个)

        uint32_t updatePeriod_; // 游戏对象update方法调用周期，单位毫秒，0表示不进行update

        bool enableLobbyClient_; // 是否需要连接Lobby服（主动调用Lobby服的rpc接口，比如：角色退出场景返回大厅）
        bool enableSceneClient_; // 是否需要连接其他Scene服（主动调用其他Scene服的rpc接口，比如：角色进行场景切换）

        std::string zooPath_;
        
    private:
        SceneConfig() = default;                                // ctor hidden
        SceneConfig(SceneConfig const&) = delete;               // copy ctor hidden
        SceneConfig(SceneConfig &&) = delete;                   // move ctor hidden
        SceneConfig& operator=(SceneConfig const&) = delete;    // assign op. hidden
        SceneConfig& operator=(SceneConfig &&) = delete;        // move assign op. hidden
        ~SceneConfig() = default;                               // dtor hidden
    };

}

#define g_SceneConfig wukong::SceneConfig::Instance()

#endif /* wukong_scene_config_h */

/*
 * Created by Xianke Liu on 2020/11/16.
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

#ifndef wukong_gateway_config_h
#define wukong_gateway_config_h

#include <stdio.h>
#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class GatewayConfig {
    public:
        struct ServerInfo {
            uint32_t id;            // 服务号（Gateway服务唯一标识，与zookeeper注册发现有关）
            uint16_t msgPort;       // 消息服务端口
            uint16_t outerPort;     // 提供给客户端的连接端口，不配置或为0时复用msgPort
        };

    public:
        static GatewayConfig& Instance() {
            static GatewayConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getInternalIp() const { return internalIp_; }
        const std::string& getExternalIp() const { return externalIp_; }
        const std::string& getOuterAddr() const { return outerAddr_; }
        uint16_t getRpcPort() const { return rpcPort_; }
        const std::vector<ServerInfo>& getServerInfos() const { return serverInfos_; }
        
        const std::string& getZookeeper() const { return zookeeper_; }
        
        uint32_t getVerifyTimeout() const { return verifyTimeout_; }
        uint32_t getDisconnectTimeout() const { return disconnectTimeout_; }
        uint32_t getIoRecvThreadNum() const { return ioRecvThreadNum_; }
        uint32_t getIoSendThreadNum() const { return ioSendThreadNum_; }

        const std::vector<RedisInfo>& getRedisInfos() const { return redisInfos_; }

        const std::string& getCoreCache() const { return coreCache_; }
        
        bool enableSceneClient() const { return enableSceneClient_; }

        const std::string& getZooPath() const { return zooPath_; }

    private:
        std::string internalIp_;    // 提供rpc服务的ip
        std::string externalIp_;    // 对客户端提供服务的ip
        std::string outerAddr_;     // 【通过NAT端口转发的服务器部署方式】提供给客户端的连接地址，不配置或为空时复用externalIp
        uint16_t rpcPort_;          // rpc服务端口

        std::vector<ServerInfo> serverInfos_; // 对外服务的信息列表
        
        std::string zookeeper_;
        
        uint32_t verifyTimeout_;        // 身份校验超时（秒）
        uint32_t disconnectTimeout_;    // 断线保护时长（秒）
        uint32_t ioRecvThreadNum_;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t ioSendThreadNum_;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        std::vector<RedisInfo> redisInfos_; // Redis库配置

        std::string coreCache_;  // 用作游戏服务器核心缓存redis库(redis中的一个)

        bool enableSceneClient_; // 是否需要连接Scene服

        std::string zooPath_;

    private:
        GatewayConfig() = default;                            // ctor hidden
        GatewayConfig(GatewayConfig const&) = delete;            // copy ctor hidden
        GatewayConfig(GatewayConfig &&) = delete;       // move ctor hidden
        GatewayConfig& operator=(GatewayConfig const&) = delete; // assign op. hidden
        GatewayConfig& operator=(GatewayConfig &&) = delete; // move assign op. hidden
        ~GatewayConfig() = default;                           // dtor hidden
    };

}

#define g_GatewayConfig wukong::GatewayConfig::Instance()

#endif /* wukong_gateway_config_h */

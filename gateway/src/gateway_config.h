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

#ifndef gateway_config_h
#define gateway_config_h

#include <stdio.h>
#include <string>
#include <vector>

namespace wukong {

    // 单例模式实现
    class GatewayConfig {
    public:
        struct ServerInfo {
            uint32_t id;            // 服务号（Gateway服务唯一标识，与zookeeper注册发现有关）
            uint16_t msgPort;       // 消息服务端口
            uint16_t outerPort;     // 提供给客户端的连接端口，不配置或为0时复用msgPort
        };

        struct RedisInfo {
            std::string host;       // db服务器host
            std::string pwd;
            uint16_t port;          // db服务器port
            uint16_t dbIndex;       // db分库索引
            uint16_t maxConnect;    // 最大连接数
        };

    public:
        static GatewayConfig& Instance() {
            static GatewayConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getInternalIp() const { return _internalIp; }
        const std::string& getExternalIp() const { return _externalIp; }
        const std::string& getOuterAddr() const { return _outerAddr; }
        uint16_t getRpcPort() const { return _rpcPort; }
        const std::vector<ServerInfo>& getServerInfos() const { return _serverInfos; }
        
        const std::string& getZookeeper() const { return _zookeeper; }
        
        uint32_t getVerifyTimeout() const { return _verifyTimeout; }
        uint32_t getDisconnectTimeout() const { return _disconnectTimeout; }
        uint32_t getIoRecvThreadNum() const { return _ioRecvThreadNum; }
        uint32_t getIoSendThreadNum() const { return _ioSendThreadNum; }

        const RedisInfo& getCache() const { return _cache; }
        
        bool enableSceneClient() const { return _enableSceneClient; }

        const std::string& getZooPath() const { return _zooPath; }

    private:
        std::string _internalIp;    // 提供rpc服务的ip
        std::string _externalIp;    // 对客户端提供服务的ip
        std::string _outerAddr;     // 【通过NAT端口转发的服务器部署方式】提供给客户端的连接地址，不配置或为空时复用externalIp
        uint16_t _rpcPort;          // rpc服务端口

        std::vector<ServerInfo> _serverInfos; // 对外服务的信息列表
        
        std::string _zookeeper;
        
        uint32_t _verifyTimeout;        // 身份校验超时（秒）
        uint32_t _disconnectTimeout;    // 断线保护时长（秒）
        uint32_t _ioRecvThreadNum;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t _ioSendThreadNum;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        RedisInfo _cache; // 缓存库配置

        bool _enableSceneClient; // 是否需要连接Scene服

        std::string _zooPath;

    private:
        GatewayConfig() = default;                            // ctor hidden
        GatewayConfig(GatewayConfig const&) = delete;            // copy ctor hidden
        GatewayConfig(GatewayConfig &&) = delete;       // move ctor hidden
        GatewayConfig& operator=(GatewayConfig const&) = delete; // assign op. hidden
        GatewayConfig& operator=(GatewayConfig &&) = delete; // move assign op. hidden
        ~GatewayConfig() = default;                           // dtor hidden
    };

    #define g_GatewayConfig GatewayConfig::Instance()

}

#endif /* gateway_config_h */

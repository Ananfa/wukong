/*
 * Created by Xianke Liu on 2021/5/6.
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

#ifndef wukong_record_config_h
#define wukong_record_config_h

#include <stdio.h>
#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class RecordConfig {
    public:
        struct ServerInfo {
            uint32_t id;            // 服务号（Record服务唯一标识，与zookeeper注册发现有关）
            // 目前只有一个id，可以根据需要扩展
        };

    public:
        static RecordConfig& Instance() {
            static RecordConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getIp() const { return _ip; }
        uint16_t getPort() const { return _port; }
        const std::vector<ServerInfo>& getServerInfos() const { return _serverInfos; }
        
        const std::string& getZookeeper() const { return _zookeeper; }
        
        uint32_t getIoRecvThreadNum() const { return _ioRecvThreadNum; }
        uint32_t getIoSendThreadNum() const { return _ioSendThreadNum; }

        const std::vector<RedisInfo>& getRedisInfos() const { return _redisInfos; }
        const std::vector<MysqlInfo>& getMysqlInfos() const { return _mysqlInfos; }

        const std::string& getCoreCache() const { return _coreCache; }
        const std::string& getCoreRecord() const { return _coreRecord; }
        
        const std::string& getZooPath() const { return _zooPath; }

    private:
        std::string _ip;    // 提供rpc服务的ip
        uint16_t _port;     // rpc服务端口

        std::vector<ServerInfo> _serverInfos; // 对外服务的信息列表
        
        std::string _zookeeper;
        
        uint32_t _ioRecvThreadNum;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t _ioSendThreadNum;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        std::vector<RedisInfo> _redisInfos; // Redis库配置
        std::vector<MysqlInfo> _mysqlInfos;

        std::string _coreCache;  // 用作游戏服务器核心缓存redis库(redis中的一个)
        std::string _coreRecord;  // 用作游戏服务器核心落地mysql库(mysql中的一个)

        std::string _zooPath;

    private:
        RecordConfig() = default;                                 // ctor hidden
        RecordConfig(RecordConfig const&) = delete;               // copy ctor hidden
        RecordConfig(RecordConfig &&) = delete;                   // move ctor hidden
        RecordConfig& operator=(RecordConfig const&) = delete;    // assign op. hidden
        RecordConfig& operator=(RecordConfig &&) = delete;        // move assign op. hidden
        ~RecordConfig() = default;                                // dtor hidden
    };

    #define g_RecordConfig RecordConfig::Instance()

}

#endif /* wukong_record_config_h */

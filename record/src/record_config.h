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

#ifndef record_config_h
#define record_config_h

#include <stdio.h>
#include <string>
#include <vector>

namespace wukong {

    // 单例模式实现
    class RecordConfig {
    public:
        struct ServerInfo {
            uint32_t id;            // 服务号（Gateway服务唯一标识，与zookeeper注册发现有关）
            uint16_t rpcPort;       // rpc服务端口
        };

        struct RedisInfo {
            std::string host;       // db服务器host
            uint16_t port;          // db服务器port
            uint16_t dbIndex;       // db分库索引
            uint16_t maxConnect;    // 最大连接数
        };

        struct MysqlInfo {
            std::string host;
            uint16_t port;
            std::string user;
            std::string pwd;
            uint16_t maxConnect;    // 最大连接数
            std::string dbName;
        };

    public:
        static RecordConfig& Instance() {
            static RecordConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getIp() const { return _ip; }
        const std::vector<ServerInfo>& getServerInfos() const { return _serverInfos; }
        
        const std::string& getZookeeper() const { return _zookeeper; }
        
        uint32_t getIoRecvThreadNum() const { return _ioRecvThreadNum; }
        uint32_t getIoSendThreadNum() const { return _ioSendThreadNum; }

        const RedisInfo& getCache() const { return _cache; }
        const MysqlInfo& getDB() const { return _db; }
        
    private:
        std::string _ip;    // 提供rpc服务的ip

        std::vector<ServerInfo> _serverInfos; // 对外服务的信息列表
        
        std::string _zookeeper;
        
        uint32_t _ioRecvThreadNum;      // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t _ioSendThreadNum;      // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        RedisInfo _cache; // 缓存库配置
        MysqlInfo _db; // 数据库配置

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

#endif /* record_config_h */

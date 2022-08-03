/*
 * Created by Xianke Liu on 2020/11/20.
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

#ifndef wukong_login_config_h
#define wukong_login_config_h

#include <string>
#include <vector>
#include "share/define.h"

namespace wukong {

    // 单例模式实现
    class LoginConfig {
    public:
        static LoginConfig& Instance() {
            static LoginConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);

        uint32_t getId() const { return _id; }
        
        const std::string& getServiceIp() const { return _serviceIp; }
        const uint16_t getServicePort() const { return _servicePort; }
        
        const std::string& getZookeeper() const { return _zookeeper; }
        
        uint32_t getWorkerThreadNum() const { return _workerThreadNum; }
        uint32_t getIoRecvThreadNum() const { return _ioRecvThreadNum; }
        uint32_t getIoSendThreadNum() const { return _ioSendThreadNum; }

        uint32_t getPlayerRoleNumInOneServer() const { return _playerRoleNumInOneServer; }
        uint32_t getPlayerRoleNumInAllServer() const { return _playerRoleNumInAllServer; }

        const std::vector<RedisInfo>& getRedisInfos() const { return _redisInfos; }
        const std::vector<MysqlInfo>& getMysqlInfos() const { return _mysqlInfos; }

        const std::string& getCoreCache() const { return _coreCache; }
        const std::string& getCorePersist() const { return _corePersist; }
        const std::string& getCoreRecord() const { return _coreRecord; }
        
        const std::string& getZooPath() const { return _zooPath; }

    private:
        uint32_t _id;       // 服务号（Login服唯一标识，与zookeeper注册发现有关）
        std::string _serviceIp;    // 对外提供http服务的ip
        uint16_t _servicePort;     // 对外提供http服务的端口

        std::string _zookeeper;
        
        uint32_t _workerThreadNum;  // http处理线程数（为0时表示在主线程进行http处理）
        uint32_t _ioRecvThreadNum;  // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t _ioSendThreadNum;  // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）

        uint32_t _playerRoleNumInOneServer; // 每个玩家在一个区服中可创建的角色数量
        uint32_t _playerRoleNumInAllServer; // 每个玩家在全区全服中可创建的角色数量
        
        // 缓存路由对象、游戏对象和存储对象对应的存在锁，玩家的session，缓存角色数据，角色轮廓数据，发布订阅等
        // 存储玩家的openid对应userid关系，userid对应roleid列表关系，逻辑服与服务器组对应关系等
        std::vector<RedisInfo> _redisInfos; // Redis库配置
        std::vector<MysqlInfo> _mysqlInfos; // mysql落地存储角色主体数据（因角色数据量大，存到redis中不太合适）

        std::string _coreCache;  // 用作游戏服务器核心缓存redis库(redis中的一个)
        std::string _corePersist;  // 用作游戏服务器核心落地redis库(redis中的一个)
        std::string _coreRecord;  // 用作游戏服务器核心落地mysql库(mysql中的一个)

        std::string _zooPath;
        
    private:
        LoginConfig() = default;                            // ctor hidden
        LoginConfig(LoginConfig const&) = delete;            // copy ctor hidden
        LoginConfig(LoginConfig &&) = delete;       // move ctor hidden
        LoginConfig& operator=(LoginConfig const&) = delete; // assign op. hidden
        LoginConfig& operator=(LoginConfig &&) = delete; // move assign op. hidden
        ~LoginConfig() = default;                           // dtor hidden
    };

    #define g_LoginConfig LoginConfig::Instance()

}

#endif /* wukong_login_config_h */

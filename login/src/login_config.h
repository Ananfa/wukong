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

#ifndef login_config_h
#define login_config_h

#include <string>

namespace wukong {

    // 单例模式实现
    class LoginConfig {
    public:
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

        uint32_t getRoleNumForPlayer() const { return _roleNumForPlayer; }

        const RedisInfo& getCache() const { return _cache; }
        const RedisInfo& getRedis() const { return _redis; }
        const MysqlInfo& getMysql() const { return _mysql; }

    private:
        uint32_t _id;       // 服务号（Login服唯一标识，与zookeeper注册发现有关）
        std::string _serviceIp;    // 对外提供http服务的ip
        uint16_t _servicePort;     // 对外提供http服务的端口

        std::string _zookeeper;
        
        uint32_t _workerThreadNum;  // http处理线程数（为0时表示在主线程进行http处理）
        uint32_t _ioRecvThreadNum;  // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t _ioSendThreadNum;  // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）

        uint32_t _roleNumForPlayer; // 每个玩家在一个区服中可创建的角色数量
        
        RedisInfo _cache; // 缓存路由对象、游戏对象和存储对象对应的存在锁，玩家的session，缓存角色数据，角色轮廓数据，发布订阅等
        RedisInfo _redis; // 存储玩家的openid对应userid关系，userid对应roleid列表关系，逻辑服与服务器组对应关系等
        MysqlInfo _mysql; // mysql落地存储角色主体数据（因角色数据量大，存到redis中不太合适）
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

#endif /* login_config_h */

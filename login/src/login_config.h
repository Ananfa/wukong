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
        

    public:
        static LoginConfig& Instance() {
            static LoginConfig theSingleton;
            return theSingleton;
        }
        
        bool parse(const char *path);
        
        const std::string& getServiceIp() const { return _serviceIp; }
        const uint16_t getServicePort() const { return _servicePort; }
        
        const std::string& getZookeeper() const { return _zookeeper; }
        const std::string& getZookeeperPath() const { return _zookeeperPath; }
        
        uint32_t getWorkerThreadNum() const { return _workerThreadNum; }
        uint32_t getIoRecvThreadNum() const { return _ioRecvThreadNum; }
        uint32_t getIoSendThreadNum() const { return _ioSendThreadNum; }

        const RedisInfo& getCache() const { return _cache; }
        const RedisInfo& getDB() const { return _db; }

    private:
        uint32_t _id;       // 服务号（Login服唯一标识，与zookeeper注册发现有关）
        std::string _serviceIp;    // 对外提供http服务的ip
        uint16_t _servicePort;     // 对外提供http服务的端口

        std::string _zookeeper;
        std::string _zookeeperPath;
        
        uint32_t _workerThreadNum;  // http处理线程数（为0时表示在主线程进行http处理）
        uint32_t _ioRecvThreadNum;  // IO接收线程数（为0表示在主线程中进行IO接收，注意：接收和发送不能都在主线程中）
        uint32_t _ioSendThreadNum;  // IO发送线程数（为0表示在主线程中进行IO发送，注意：接收和发送不能都在主线程中）
        
        RedisInfo _cache;
        RedisInfo _db;
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

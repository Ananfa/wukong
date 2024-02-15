/*
 * Created by Xianke Liu on 2022/2/24.
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

#ifndef wukong_mysql_pool_h
#define wukong_mysql_pool_h

#include "corpc_mysql.h"

#include <map>
#include <string>

using namespace corpc;

namespace wukong {

    class MysqlPool {
    public:
        static MysqlPool* create(const char *host, const char *user, const char *pwd, const char *dbName, unsigned int port, const char *unix_socket, unsigned long clientflag, uint32_t maxConnectNum);
    
    public:
        MysqlConnectPool *getPool() { return mysql_; }

        MYSQL *take();
        void put(MYSQL *mysql, bool error);

    private:
        MysqlPool() {}
        virtual ~MysqlPool() {}

        void init(const char *host, const char *user, const char *pwd, const char *dbName, unsigned int port, const char *unix_socket, unsigned long clientflag, uint32_t maxConnectNum);

    private:
        MysqlConnectPool *mysql_;
    };

    // TODO: 进行分片（sharding）设计，应配置分片数量，提供计算所属分片的方法（支持参数为字符串或整数）
    class MysqlPoolManager {
    public:
        static MysqlPoolManager& Instance() {
            static MysqlPoolManager instance;
            return instance;
        }

        bool addPool(const std::string &poolName, MysqlPool* pool); // 注意：此方法不是线程安全的，应该在服务器启动时调用
        MysqlPool *getPool(const std::string &poolName);

        bool setCoreRecord(const std::string &poolName);
        MysqlPool *getCoreRecord() { return coreRecord_; }

    private:
        std::map<std::string, MysqlPool*> poolMap_;

        MysqlPool *coreRecord_ = nullptr;

    private:
        MysqlPoolManager() = default;                                       // ctor hidden
        ~MysqlPoolManager() = default;                                      // destruct hidden
        MysqlPoolManager(MysqlPoolManager const&) = delete;                 // copy ctor delete
        MysqlPoolManager(MysqlPoolManager &&) = delete;                     // move ctor delete
        MysqlPoolManager& operator=(MysqlPoolManager const&) = delete;      // assign op. delete
        MysqlPoolManager& operator=(MysqlPoolManager &&) = delete;          // move assign op. delete
    };
}

#define g_MysqlPoolManager wukong::MysqlPoolManager::Instance()

#endif /* wukong_mysql_pool_h */

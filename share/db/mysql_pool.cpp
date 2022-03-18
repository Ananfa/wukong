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

#include "mysql_pool.h"

using namespace wukong;

MysqlPool* MysqlPool::create(const char *host, const char *user, const char *pwd, const char *dbName, unsigned int port, const char *unix_socket, unsigned long clientflag, uint32_t maxConnectNum) {
	MysqlPool* pool = new MysqlPool();
    pool->init(host, user, pwd, dbName, port, unix_socket, clientflag, maxConnectNum);

    return pool;
}
    
void MysqlPool::init(const char *host, const char *user, const char *pwd, const char *dbName, unsigned int port, const char *unix_socket, unsigned long clientflag, uint32_t maxConnectNum) {
    _mysql = corpc::MysqlConnectPool::create(host, user, pwd, dbName, port, unix_socket, clientflag, maxConnectNum);
}

MYSQL *MysqlPool::take() {
	return _mysql->proxy.take();
}

void MysqlPool::put(MYSQL *mysql, bool error) {
	_mysql->proxy.put(mysql, error);
}

bool MysqlPoolManager::addPool(const std::string &poolName, MysqlPool* pool) {
    if (_poolMap.find(poolName) != _poolMap.end()) {
        ERROR_LOG("MysqlPoolManager::addPool -- multiple set pool[%s]\n", poolName.c_str());
        return false;
    }

    _poolMap.insert(std::make_pair(poolName, pool));
    return true;
}

MysqlPool *MysqlPoolManager::getPool(const std::string &poolName) {
	auto it = _poolMap.find(poolName);
    if (it == _poolMap.end()) {
        return nullptr;
    }

    return it->second;
}

bool MysqlPoolManager::setCoreRecord(const std::string &poolName) {
    if (_coreRecord != nullptr) {
        ERROR_LOG("MysqlPoolManager::setCoreRecord -- multiple set core-record pool[%s]\n", poolName.c_str());
        return false;
    }

    auto it = _poolMap.find(poolName);
    if (it == _poolMap.end()) {
        ERROR_LOG("MysqlPoolManager::setCoreRecord -- pool[%s] not exist\n", poolName.c_str());
        return false;
    }

    _coreRecord = it->second;
    return true;
}

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

void MysqlPool::init(const char *host, const char *user, const char *pwd, const char *dbName, unsigned int port, const char *unix_socket, unsigned long clientflag, uint32_t maxConnectNum) {
    _mysql = corpc::MysqlConnectPool::create(host, user, pwd, dbName, port, unix_socket, clientflag, maxConnectNum);
}

MYSQL *MysqlPool::take() {
	return _mysql->proxy.take();
}

void MysqlPool::put(MYSQL *mysql, bool error) {
	_mysql->proxy.put(mysql, error);
}

/*
 * Created by Xianke Liu on 2021/6/7.
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

#include "login_server.h"
#include "corpc_routine_env.h"
#include "corpc_pubsub.h"

#include "login_config.h"
#include "const.h"
#include "utility.h"

#include "login_handler_mgr.h"
#include "client_center.h"

#include "redis_pool.h"
#include "mysql_pool.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

bool LoginServer::init(int argc, char * argv[]) {
    if (inited_) {
        return false;
    }

    inited_ = true;

    RoutineEnvironment::init();

    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigaction( SIGPIPE, &sa, NULL );
    
    char *configFileName = NULL;
    
    // parse args, config filename in args
    int c;
    while ((c = getopt(argc, argv, "c:l:")) != -1) {
        switch (c) {
            case 'c':
                configFileName = optarg;
                break;
                
            case 'l':
                if (!Utility::mkdirp(optarg)) {
                    ERROR_LOG("Can't mkdir %s\n", optarg);
                    return false;
                }
                
                setLogPath(optarg);
                break;
                
            default:
                break;
        }
    }
    
    if (!configFileName) {
        ERROR_LOG("Please start with '-c configFile' argument\n");
        return false;
    }
    
    // check file exist
    struct stat buffer;
    if (stat(configFileName, &buffer) != 0) {
        ERROR_LOG("Can't open file %s for %d:%s\n", configFileName, errno, strerror(errno));
        return false;
    }
    
    // parse config file content to config object
    if (!g_LoginConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    io_ = IO::create(g_LoginConfig.getIoRecvThreadNum(), g_LoginConfig.getIoSendThreadNum());
    
    // 初始化rpc clients
    rpcClient_ = RpcClient::create(io_);

    // 启动http服务
    _httpServer = HttpServer::create(io_, g_LoginConfig.getWorkerThreadNum(), g_LoginConfig.getServiceIp(), g_LoginConfig.getServicePort());

    // 数据库初始化
    const std::vector<RedisInfo>& redisInfos = g_LoginConfig.getRedisInfos();
    for (auto &info : redisInfos) {
        RedisPool *pool = RedisPool::create(info.host.c_str(), info.pwd.c_str(), info.port, info.dbIndex, info.maxConnect);
        if (!g_RedisPoolManager.addPool(info.dbName, pool)) {
            ERROR_LOG("LoginServer::init -- addPool[%s] failed\n", info.dbName.c_str());
            return false;
        }
    }

    const std::vector<MysqlInfo>& mysqlInfos = g_LoginConfig.getMysqlInfos();
    for (auto &info : mysqlInfos) {
        MysqlPool *pool = MysqlPool::create(info.host.c_str(), info.user.c_str(), info.pwd.c_str(), info.dbName.c_str(), info.port, "", 0, info.maxConnect);
        if (!g_MysqlPoolManager.addPool(info.dbName, pool)) {
            ERROR_LOG("LoginServer::init -- addPool[%s] failed\n", info.dbName.c_str());
            return false;
        }
    }

    g_RedisPoolManager.setCoreCache(g_LoginConfig.getCoreCache());
    g_RedisPoolManager.setCorePersist(g_LoginConfig.getCoreCache());
    g_MysqlPoolManager.setCoreRecord(g_LoginConfig.getCoreRecord());

    // 初始化发布订阅服务
    PubsubService::StartPubsubService(g_RedisPoolManager.getCoreCache()->getPool());

    return true;
}

void LoginServer::run() {
    g_LoginHandlerMgr.init(_httpServer);
    g_ClientCenter.init(rpcClient_, g_LoginConfig.getZookeeper(), g_LoginConfig.getZooPath(), true, false, false, false);
    RoutineEnvironment::runEventLoop();
}
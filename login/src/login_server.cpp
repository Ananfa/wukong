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

#include "login_config.h"
#include "const.h"
#include "utility.h"

#include "login_handler_mgr.h"
#include "client_center.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

bool LoginServer::init(int argc, char * argv[]) {
    if (_inited) {
        return false;
    }

    _inited = true;

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
    _io = IO::create(g_LoginConfig.getIoRecvThreadNum(), g_LoginConfig.getIoSendThreadNum());
    
    // 初始化rpc clients
    _rpcClient = RpcClient::create(_io);

    // 启动http服务
    _httpServer = HttpServer::create(_io, g_LoginConfig.getWorkerThreadNum(), g_LoginConfig.getServiceIp(), g_LoginConfig.getServicePort());

    return true;
}

void LoginServer::run() {
    g_LoginHandlerMgr.init(_httpServer);
    g_ClientCenter.init(_rpcClient, g_LoginConfig.getZookeeper(), g_LoginConfig.getZooPath(), true, false, false, false);
    RoutineEnvironment::runEventLoop();
}
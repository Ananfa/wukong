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
#include "zk_client.h"
#include "const.h"
#include "utility.h"

#include "gateway_client.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

void LoginServer::enterZoo() {
    g_ZkClient.init(g_LoginConfig.getZookeeper(), ZK_TIMEOUT, []() {
        std::string zooPath = ZK_LOGIN_SERVER + "/" + std::to_string(g_LoginConfig.getId()) + "|" + g_LoginConfig.getServiceIp() + ":" + std::to_string(g_LoginConfig.getServicePort());
        g_ZkClient.createEphemeralNode(zooPath, ZK_DEFAULT_VALUE, [](const std::string &path, const ZkRet &ret) {
            if (ret) {
                LOG("create rpc node:[%s] sucessful\n", path.c_str());
            } else {
                ERROR_LOG("create rpc node:[%d] failed, code = %d\n", path.c_str(), ret.code());
            }
        });

        g_ZkClient.watchChildren(ZK_GATEWAY_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
            std::map<uint16_t, GatewayClient::AddressInfo> addresses;
            for (const std::string &value : values) {
                GatewayClient::AddressInfo address;

                if (GatewayClient::parseAddress(value, address)) {
                    addresses[address.id] = std::move(address);
                } else {
                    ERROR_LOG("zkclient parse gateway server address error, info = %s\n", value.c_str());
                }
            }
            g_GatewayClient.setServers(addresses);
        });
    });
}

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
    enterZoo();
    RoutineEnvironment::runEventLoop();
}
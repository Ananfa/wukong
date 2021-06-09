/*
 * Created by Xianke Liu on 2021/6/9.
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

#include "record_server.h"
#include "corpc_routine_env.h"
#include "corpc_rpc_server.h"

#include "record_config.h"
#include "record_service.h"

#include "zk_client.h"
#include "utility.h"
#include "share/const.h"

#include <sys/stat.h>

using namespace corpc;
using namespace wukong;


void RecordServer::enterZoo() {
    g_ZkClient.init(g_RecordConfig.getZookeeper(), ZK_TIMEOUT, []() {
        // 对servers配置中每一个server进行节点注册
        const std::vector<RecordConfig::ServerInfo> &serverInfos = g_RecordConfig.getServerInfos();
        for (const RecordConfig::ServerInfo &info : serverInfos) {
            std::string zooPath = ZK_RECORD_SERVER + "/" + std::to_string(info.id) + "|" + g_RecordConfig.getIp() + ":" + std::to_string(info.rpcPort);
            g_ZkClient.createEphemeralNode(zooPath, ZK_DEFAULT_VALUE, [](const std::string &path, const ZkRet &ret) {
                if (ret) {
                    LOG("create rpc node:[%s] sucessful\n", path.c_str());
                } else {
                    ERROR_LOG("create rpc node:[%d] failed, code = %d\n", path.c_str(), ret.code());
                }
            });
        }
    });
}

void RecordServer::recordThread(IO *rpc_io, ServerId rcid, uint16_t rpcPort) {
    // 启动RPC服务
    RpcServer *server = RpcServer::create(rpc_io, 0, g_RecordConfig.getIp(), rpcPort);
    
    RecordManager *mgr = new RecordManager(rcid);
    mgr->init();

    RecordServiceImpl *recordServiceImpl = new RecordServiceImpl(mgr);
    server->registerService(recordServiceImpl);

    RoutineEnvironment::runEventLoop();
}

bool RecordServer::init(int argc, char * argv[]) {
    if (_inited) {
        return false;
    }

    _inited = true;

    co_start_hook();

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
    if (!g_RecordConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    _io = IO::create(g_RecordConfig.getIoRecvThreadNum(), g_RecordConfig.getIoSendThreadNum());

    return true;
}

void RecordServer::run() {
    // 根据servers配置启动Record服务，每线程跑一个服务
    const std::vector<RecordConfig::ServerInfo> &recordInfos = g_RecordConfig.getServerInfos();
    for (auto &info : recordInfos) {
        _threads.push_back(std::thread(recordThread, _io, info.id, info.rpcPort));
    }

    enterZoo();

    RoutineEnvironment::runEventLoop();
}
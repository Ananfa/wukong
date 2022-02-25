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
#include "record_center.h"
#include "cache_pool.h"
#include "mysql_pool.h"
#include "client_center.h"

#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

void RecordServer::recordThread(InnerRpcServer *server, ServerId rcid) {
    // 启动RPC服务
    server->start(false);
    
    RecordObjectManager *mgr = new RecordObjectManager(rcid);
    mgr->init();

    InnerRecordServiceImpl *recordServiceImpl = new InnerRecordServiceImpl(mgr);
    server->registerService(recordServiceImpl);

    RoutineEnvironment::runEventLoop();
}

bool RecordServer::init(int argc, char * argv[]) {
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
    if (!g_RecordConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    _io = IO::create(g_RecordConfig.getIoRecvThreadNum(), g_RecordConfig.getIoSendThreadNum());

    return true;
}

void RecordServer::run() {
    RecordServiceImpl *recordServiceImpl = new RecordServiceImpl();

    // 根据servers配置启动Record服务，每线程跑一个服务
    const std::vector<RecordConfig::ServerInfo> &recordInfos = g_RecordConfig.getServerInfos();
    for (auto &info : recordInfos) {
        // 创建内部RPC服务
        InnerRpcServer *innerServer = new InnerRpcServer();

        recordServiceImpl->addInnerStub(info.id, new pb::InnerRecordService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));

        _threads.push_back(std::thread(recordThread, innerServer, info.id));
    }

    // 启动对外的RPC服务
    RpcServer *server = RpcServer::create(_io, 0, g_RecordConfig.getIp(), g_RecordConfig.getPort());
    server->registerService(recordServiceImpl);

    g_CachePool.init(g_RecordConfig.getCache().host.c_str(), g_RecordConfig.getCache().pwd.c_str(), g_RecordConfig.getCache().port, g_RecordConfig.getCache().dbIndex, g_RecordConfig.getCache().maxConnect);
    g_MysqlPool.init(g_RecordConfig.getMysql().host.c_str(), g_RecordConfig.getMysql().user.c_str(), g_RecordConfig.getMysql().pwd.c_str(), g_RecordConfig.getMysql().dbName.c_str(), g_RecordConfig.getMysql().port, "", 0, g_RecordConfig.getMysql().maxConnect);

    g_RecordCenter.init();
    g_ClientCenter.init(nullptr, g_RecordConfig.getZookeeper(), g_RecordConfig.getZooPath(), false, false, false, false);
    RoutineEnvironment::runEventLoop();
}

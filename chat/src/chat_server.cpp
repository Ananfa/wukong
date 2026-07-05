/*
 * Created by Xianke Liu on 2025/5/21.
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

#include "chat_server.h"
#include "corpc_routine_env.h"
#include "corpc_rpc_server.h"
#include "corpc_pubsub.h"

#include "chat_config.h"
#include "chat_service.h"
//#include "message_handle_manager.h"
#include "agent_manager.h"
#include "gateway_agent.h"

#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

bool ChatServer::init(int argc, char * argv[]) {
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
    if (!g_ChatConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }

    // create IO layer
    io_ = IO::create(g_ChatConfig.getIoRecvThreadNum(), g_ChatConfig.getIoSendThreadNum(), 0);

    // 初始化rpc clients
    rpcClient_ = RpcClient::create(io_);

    g_AgentManager.registerAgent(new GatewayAgent(rpcClient_));

    pb::ServerInfo serverInfo;
    serverInfo.set_server_type(SERVER_TYPE_CHAT);
    serverInfo.set_rpc_host(g_ChatConfig.getIp());
    serverInfo.set_rpc_port(g_ChatConfig.getPort());
    if (!g_AgentManager.init(io_, g_ChatConfig.getNexusAddr().host, g_ChatConfig.getNexusAddr().port, serverInfo)) {
        ERROR_LOG("agent manager init failed\n");
        return false;
    }

    //g_MessageHandleManager;

    return true;
}

void ChatServer::run() {
    // 启动对外的RPC服务
    RpcServer *server = RpcServer::create(io_, nullptr, g_ChatConfig.getIp(), g_ChatConfig.getPort());
    server->registerService(new ChatServiceImpl());

    g_AgentManager.start();

    RoutineEnvironment::runEventLoop();
}
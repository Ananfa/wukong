/*
 * Created by Xianke Liu on 2020/11/16.
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

#include "corpc_routine_env.h"
#include "corpc_rpc_client.h"
#include "corpc_rpc_server.h"
#include "corpc_message_server.h"

#include "gateway_config.h"
#include "gateway_service.h"
#include "gateway_manager.h"
#include "gateway_center.h"
#include "gateway_handler.h"

#include "lobby_client.h"
#include "zk_client.h"
#include "const.h"
#include "define.h"
#include "utility.h"

#include <thread>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

void enterZoo() {
    g_ZkClient.init(g_GatewayConfig.getZookeeper(), ZK_TIMEOUT, []() {
        // 对servers配置中每一个server进行节点注册
        const std::vector<GatewayConfig::ServerInfo> &serverInfos = g_GatewayConfig.getServerInfos();
        for (const GatewayConfig::ServerInfo &info : serverInfos) {
            std::string zooPath = ZK_GATEWAY_SERVER + "/" + std::to_string(info.id) + "|" + g_GatewayConfig.getInternalIp() + ":" + std::to_string(info.rpcPort) + "|" + info.outerAddr + ":" + std::to_string(info.msgPort);
            g_ZkClient.createEphemeralNode(zooPath, ZK_DEFAULT_VALUE, [](const std::string &path, const ZkRet &ret) {
                if (ret) {
                    LOG("create rpc node:[%s] sucessful\n", path.c_str());
                } else {
                    ERROR_LOG("create rpc node:[%d] failed, code = %d\n", path.c_str(), ret.code());
                }
            });
        }

        g_ZkClient.watchChildren(ZK_LOBBY_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
            std::map<uint16_t, LobbyClient::AddressInfo> addresses;
            for (const std::string &value : values) {
                LobbyClient::AddressInfo address;

                if (LobbyClient::parseAddress(value, address)) {
                    addresses[address.id] = std::move(address);
                } else {
                    ERROR_LOG("zkclient parse lobby server address error, info = %s\n", value.c_str());
                }
            }
            g_LobbyClient.setServers(addresses);
        });
        
        // TODO: watch other servers
    });
}

void gatewayThread(IO *rpc_io, IO *msg_io, ServerId gwid, uint16_t rpcPort, uint16_t msgPort) {
    // 启动RPC服务
    RpcServer *server = RpcServer::create(rpc_io, 0, g_GatewayConfig.getInternalIp(), rpcPort);
    
    GatewayManager *mgr = new GatewayManager(gwid);
    mgr->init();

    GatewayServiceImpl *gatewayServiceImpl = new GatewayServiceImpl(mgr);
    //GatewayTransitServiceImpl *transitServiceImpl = new GatewayTransitServiceImpl(mgr);
    server->registerService(gatewayServiceImpl);
    //server->registerService(transitServiceImpl);

    // 启动消息服务，注册连接建立、断开、消息禁止、身份认证等消息处理，以及设置旁路处理（将消息转发给玩家游戏对象所在的服务器）
    corpc::TcpMessageServer *msgServer = new corpc::TcpMessageServer(msg_io, true, true, true, true, g_GatewayConfig.getExternalIp(), msgPort);
    msgServer->start();
    
    GatewayHandler *handler = new GatewayHandler(mgr);
    handler->registerMessages(msgServer);

    RoutineEnvironment::runEventLoop();
}

int main(int argc, char * argv[]) {
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
                    return -1;
                }
                
                setLogPath(optarg);
                break;
                
            default:
                break;
        }
    }
    
    if (!configFileName) {
        ERROR_LOG("Please start with '-c configFile' argument\n");
        return -1;
    }
    
    // check file exist
    struct stat buffer;
    if (stat(configFileName, &buffer) != 0) {
        ERROR_LOG("Can't open file %s for %d:%s\n", configFileName, errno, strerror(errno));
        return -1;
    }
    
    // parse config file content to config object
    if (!g_GatewayConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return -1;
    }
    
    // create IO layer
    IO *io = IO::create(g_GatewayConfig.getIoRecvThreadNum(), g_GatewayConfig.getIoSendThreadNum());

    // 初始化全局资源
    g_GatewayCenter.init();
    
    // 初始化rpc clients
    RpcClient *client = RpcClient::create(io);
    // lobby client初始化
    g_LobbyClient.init(client);
    // TODO: 初始化其他game client（实现game service的服务器client）

    // 根据servers配置启动Gateway服务，每线程跑一个服务
    std::vector<std::thread> gwThreads;
    const std::vector<GatewayConfig::ServerInfo> &gatewayInfos = g_GatewayConfig.getServerInfos();
    for (auto iter = gatewayInfos.begin(); iter != gatewayInfos.end(); iter++) {
        gwThreads.push_back(std::thread(gatewayThread, io, io, iter->id, iter->rpcPort, iter->msgPort));
    }

    enterZoo();

    RoutineEnvironment::runEventLoop();
}
/*
 * Created by Xianke Liu on 2021/6/8.
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

#include "gateway_server.h"
#include "corpc_routine_env.h"
#include "corpc_rpc_client.h"
#include "corpc_rpc_server.h"
#include "corpc_message_server.h"

#include "gateway_config.h"
#include "gateway_service.h"
#include "gateway_manager.h"
#include "gateway_handler.h"

#include "zk_client.h"
#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

void GatewayServer::enterZoo() {
    g_ZkClient.init(g_GatewayConfig.getZookeeper(), ZK_TIMEOUT, [this]() {
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

        for (auto &pair : _gameClientMap) {
            g_ZkClient.watchChildren(pair.second->getZkNodeName(), [pair](const std::string &path, const std::vector<std::string> &values) {
                std::map<uint16_t, GameClient::AddressInfo> addresses;
                for (const std::string &value : values) {
                    GameClient::AddressInfo address;

                    if (GameClient::parseAddress(value, address)) {
                        addresses[address.id] = std::move(address);
                    } else {
                        ERROR_LOG("zkclient parse game server[gsType:%d] address error, info = %s\n", pair.first, value.c_str());
                    }
                }
                pair.second->setServers(addresses);
            });
        }
    });
}

void GatewayServer::gatewayThread(IO *rpc_io, IO *msg_io, ServerId gwid, uint16_t rpcPort, uint16_t msgPort) {
    // 启动RPC服务
    RpcServer *server = RpcServer::create(rpc_io, 0, g_GatewayConfig.getInternalIp(), rpcPort);
    
    GatewayManager *mgr = new GatewayManager(gwid);
    mgr->init();

    GatewayServiceImpl *gatewayServiceImpl = new GatewayServiceImpl(mgr);
    server->registerService(gatewayServiceImpl);

    // 启动消息服务，注册连接建立、断开、消息禁止、身份认证等消息处理，以及设置旁路处理（将消息转发给玩家游戏对象所在的服务器）
    corpc::TcpMessageServer *msgServer = new corpc::TcpMessageServer(msg_io, true, true, true, true, g_GatewayConfig.getExternalIp(), msgPort);
    msgServer->start();
    
    GatewayHandler *handler = new GatewayHandler(mgr);
    handler->registerMessages(msgServer);

    RoutineEnvironment::runEventLoop();
}

bool GatewayServer::init(int argc, char * argv[]) {
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
    if (!g_GatewayConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return -1;
    }
    
    // create IO layer
    _io = IO::create(g_GatewayConfig.getIoRecvThreadNum(), g_GatewayConfig.getIoSendThreadNum());

    // 初始化rpc clients
    _rpcClient = RpcClient::create(_io);

    return true;
}

void GatewayServer::run() {
    // 根据servers配置启动Gateway服务，每线程跑一个服务
    const std::vector<GatewayConfig::ServerInfo> &gatewayInfos = g_GatewayConfig.getServerInfos();
    for (auto &info : gatewayInfos) {
        _threads.push_back(std::thread(gatewayThread, _io, _io, info.id, info.rpcPort, info.msgPort));
    }

    enterZoo();
    RoutineEnvironment::runEventLoop();
}

void GatewayServer::registerGameClient(GameClient *client) {
    _gameClientMap.insert(std::make_pair(client->getGameServerType(), client));
}

GameClient *GatewayServer::getGameClient(GameServerType gsType) {
    auto it = _gameClientMap.find(gsType);

    if (it == _gameClientMap.end()) {
        return nullptr;
    }

    return it->second;
}
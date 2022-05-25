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

#include "lobby_server.h"
#include "corpc_routine_env.h"
#include "corpc_rpc_server.h"
#include "corpc_pubsub.h"

#include "lobby_config.h"
#include "lobby_service.h"
#include "game_service.h"
#include "game_center.h"
#include "client_center.h"
#include "redis_pool.h"

#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

void LobbyServer::lobbyThread(InnerRpcServer *server, ServerId lbid) {
    // 启动RPC服务
    server->start(false);
    
    GameObjectManager *mgr = new GameObjectManager(GAME_SERVER_TYPE_LOBBY, lbid);
    mgr->init();

    InnerLobbyServiceImpl *lobbyServiceImpl = new InnerLobbyServiceImpl(mgr);
    InnerGameServiceImpl *gameServiceImpl = new InnerGameServiceImpl(mgr);
    server->registerService(lobbyServiceImpl);
    server->registerService(gameServiceImpl);

    RoutineEnvironment::runEventLoop();
}

bool LobbyServer::init(int argc, char * argv[]) {
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
    if (!g_LobbyConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    _io = IO::create(g_LobbyConfig.getIoRecvThreadNum(), g_LobbyConfig.getIoSendThreadNum());

    // 初始化rpc clients
    _rpcClient = RpcClient::create(_io);

    // 数据库初始化
    const std::vector<RedisInfo>& redisInfos = g_LobbyConfig.getRedisInfos();
    for (auto &info : redisInfos) {
        RedisPool *pool = RedisPool::create(info.host.c_str(), info.pwd.c_str(), info.port, info.dbIndex, info.maxConnect);
        if (!g_RedisPoolManager.addPool(info.dbName, pool)) {
            ERROR_LOG("LobbyServer::init -- addPool[%s] failed\n", info.dbName.c_str());
            exit(EXIT_FAILURE);
        }
    }

    g_RedisPoolManager.setCoreCache(g_LobbyConfig.getCoreCache());

    // 初始化发布订阅服务
    PubsubService::StartPubsubService(g_RedisPoolManager.getCoreCache()->getPool());

    return true;
}

void LobbyServer::run() {
    // 注意：GameCenter要在game object manager之前init，因为跨服事件队列处理协程需要先启动
    g_GameCenter.init(GAME_SERVER_TYPE_LOBBY, g_LobbyConfig.getUpdatePeriod());

    // TODO: 初始化lua环境，监听消息的lua绑定信息

    LobbyServiceImpl *lobbyServiceImpl = new LobbyServiceImpl();
    GameServiceImpl *gameServiceImpl = new GameServiceImpl();

    // 根据servers配置启动Lobby服务，每线程跑一个服务
    const std::vector<LobbyConfig::ServerInfo> &lobbyInfos = g_LobbyConfig.getServerInfos();
    for (auto &info : lobbyInfos) {
        // 创建内部RPC服务
        InnerRpcServer *innerServer = new InnerRpcServer();

        lobbyServiceImpl->addInnerStub(info.id, new pb::InnerLobbyService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));
        gameServiceImpl->addInnerStub(info.id, new pb::InnerGameService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));

        _threads.push_back(std::thread(lobbyThread, innerServer, info.id));
    }

    // 启动对外的RPC服务
    RpcServer *server = RpcServer::create(_io, 0, g_LobbyConfig.getIp(), g_LobbyConfig.getPort());
    server->registerService(lobbyServiceImpl);
    server->registerService(gameServiceImpl);

    // 注意：ClientCenter在最后才init，因为服务器准备好才向zookeeper注册
    g_ClientCenter.init(_rpcClient, g_LobbyConfig.getZookeeper(), g_LobbyConfig.getZooPath(), true, true, false, g_LobbyConfig.enableSceneClient());
    RoutineEnvironment::runEventLoop();
}
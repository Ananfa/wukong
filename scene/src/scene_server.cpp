/*
 * Created by Xianke Liu on 2022/1/12.
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

#include "scene_server.h"
#include "corpc_routine_env.h"
#include "corpc_rpc_server.h"

#include "scene_config.h"
#include "scene_service.h"
#include "game_service.h"
#include "game_center.h"
#include "client_center.h"
#include "gateway_client.h"
#include "record_client.h"
#include "scene_client.h"
#include "redis_pool.h"

#include "zk_client.h"
#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

void SceneServer::sceneThread(InnerRpcServer *server, ServerId sid) {
    // 启动RPC服务
    server->start(false);
    
    SceneManager *sceneMgr = new SceneManager(sid);
    sceneMgr->init();

    InnerSceneServiceImpl *sceneServiceImpl = new InnerSceneServiceImpl(sceneMgr);
    InnerGameServiceImpl *gameServiceImpl = new InnerGameServiceImpl(sceneMgr);
    server->registerService(sceneServiceImpl);
    server->registerService(gameServiceImpl);

    RoutineEnvironment::runEventLoop();
}

bool SceneServer::init(int argc, char * argv[]) {
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
    if (!g_SceneConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    io_ = IO::create(g_SceneConfig.getIoRecvThreadNum(), g_SceneConfig.getIoSendThreadNum());

    // 初始化rpc clients
    rpcClient_ = RpcClient::create(io_);

    // 数据库初始化
    const std::vector<RedisInfo>& redisInfos = g_SceneConfig.getRedisInfos();
    for (auto &info : redisInfos) {
        RedisPool *pool = RedisPool::create(info.host.c_str(), info.pwd.c_str(), info.port, info.dbIndex, info.maxConnect);
        if (!g_RedisPoolManager.addPool(info.dbName, pool)) {
            ERROR_LOG("SceneServer::init -- addPool[%s] failed\n", info.dbName.c_str());
            return false;
        }
    }

    g_RedisPoolManager.setCoreCache(g_SceneConfig.getCoreCache());

    return true;
}

void SceneServer::run() {
    // 注意：GameCenter要在game object manager之前init，因为跨服事件队列处理协程需要先启动
    g_GameCenter.init(GAME_SERVER_TYPE_SCENE, g_SceneConfig.getUpdatePeriod());

    SceneServiceImpl *sceneServiceImpl = new SceneServiceImpl();
    GameServiceImpl *gameServiceImpl = new GameServiceImpl();

    // 根据servers配置启动Lobby服务，每线程跑一个服务
    const std::vector<SceneConfig::ServerInfo> &sceneInfos = g_SceneConfig.getServerInfos();
    for (auto &info : sceneInfos) {
        // 创建内部RPC服务
        InnerRpcServer *innerServer = new InnerRpcServer();

        sceneServiceImpl->addInnerStub(info.id, new pb::InnerSceneService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));
        gameServiceImpl->addInnerStub(info.id, new pb::InnerGameService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));

        threads_.push_back(std::thread(sceneThread, innerServer, info.id));
    }

    // 启动对外的RPC服务
    RpcServer *server = RpcServer::create(io_, 0, g_SceneConfig.getIp(), g_SceneConfig.getPort());
    server->registerService(sceneServiceImpl);
    server->registerService(gameServiceImpl);

    // TODO: 场景服是否需要g_SceneCenter负责场景相关sha1值维护？

    // 注意：ClientCenter在最后才init，因为服务器准备好才向zookeeper注册
    g_ClientCenter.init(rpcClient_, g_SceneConfig.getZookeeper(), g_SceneConfig.getZooPath(), true, true, g_SceneConfig.enableLobbyClient(), g_SceneConfig.enableSceneClient());
    RoutineEnvironment::runEventLoop();
}
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
#include "corpc_pubsub.h"

#include "gateway_config.h"
#include "gateway_service.h"
#include "gateway_object_manager.h"
#include "gateway_handler.h"
#include "client_center.h"
#include "redis_pool.h"
#include "redis_utils.h"
#include "rapidjson/document.h"

#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace rapidjson;
using namespace corpc;
using namespace wukong;

void GatewayServer::gatewayThread(InnerRpcServer *server, IO *msg_io, ServerId gwid, uint16_t msgPort) {
    // 启动RPC服务
    server->start(0);
    
    GatewayObjectManager *mgr = new GatewayObjectManager(gwid);
    mgr->init();

    InnerGatewayServiceImpl *gatewayServiceImpl = new InnerGatewayServiceImpl(mgr);
    server->registerService(gatewayServiceImpl);

    // 启动消息服务，注册连接建立、断开、消息禁止、身份认证等消息处理，以及设置旁路处理（将消息转发给玩家游戏对象所在的服务器）
    corpc::TcpMessageServer *msgServer = new corpc::TcpMessageServer(msg_io, true, true, true, true, g_GatewayConfig.getExternalIp(), msgPort);
    msgServer->start();
    
    GatewayHandler *handler = new GatewayHandler(mgr);
    handler->registerMessages(msgServer);

    // 从Redis中获取初始的消息屏蔽信息
    RoutineEnvironment::startCoroutine([](void * arg) -> void* {
        corpc::TcpMessageServer *msgServer = (corpc::TcpMessageServer *)arg;
        banMsgHandle(msgServer);
        return NULL;
    }, msgServer);

    // 用订阅主题“屏蔽”，不需要用全服事件
    //PubsubService::Subscribe("WK_BanMsg", true, std::bind(&GatewayServer::banMsgHandle, this, std::placeholders::_1, std::placeholders::_2));
    PubsubService::Subscribe("WK_BanMsg", true, [msgServer](const std::string& topic, const std::string& msg) {
        banMsgHandle(msgServer);
    });

    RoutineEnvironment::runEventLoop();
}

bool GatewayServer::init(int argc, char * argv[]) {
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
    if (!g_GatewayConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    io_ = IO::create(g_GatewayConfig.getIoRecvThreadNum(), g_GatewayConfig.getIoSendThreadNum());

    // 初始化rpc clients
    rpcClient_ = RpcClient::create(io_);

    // 数据库初始化
    const std::vector<RedisInfo>& redisInfos = g_GatewayConfig.getRedisInfos();
    for (auto &info : redisInfos) {
        RedisPool *pool = RedisPool::create(info.host.c_str(), info.pwd.c_str(), info.port, info.dbIndex, info.maxConnect);
        if (!g_RedisPoolManager.addPool(info.dbName, pool)) {
            ERROR_LOG("GatewayServer::init -- addPool[%s] failed\n", info.dbName.c_str());
            return false;
        }
    }

    g_RedisPoolManager.setCoreCache(g_GatewayConfig.getCoreCache());

    // 初始化发布订阅服务
    PubsubService::StartPubsubService(g_RedisPoolManager.getCoreCache()->getPool());

    return true;
}

void GatewayServer::run() {
    GatewayServiceImpl *gatewayServiceImpl = new GatewayServiceImpl();

    // 根据servers配置启动Gateway服务，每线程跑一个服务
    const std::vector<GatewayConfig::ServerInfo> &gatewayInfos = g_GatewayConfig.getServerInfos();
    for (auto &info : gatewayInfos) {
        // 创建内部RPC服务
        InnerRpcServer *innerServer = new InnerRpcServer();

        gatewayServiceImpl->addInnerStub(info.id, new pb::InnerGatewayService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));

        threads_.push_back(std::thread(gatewayThread, innerServer, io_, info.id, info.msgPort));
    }

    // 启动对外的RPC服务
    RpcServer *server = RpcServer::create(io_, 0, g_GatewayConfig.getInternalIp(), g_GatewayConfig.getRpcPort());
    server->registerService(gatewayServiceImpl);

    g_ClientCenter.init(rpcClient_, g_GatewayConfig.getZookeeper(), g_GatewayConfig.getZooPath(), false, false, true, g_GatewayConfig.enableSceneClient());
    RoutineEnvironment::runEventLoop();
}

void GatewayServer::banMsgHandle(corpc::TcpMessageServer *msgServer) {
    // 从Redis中查出最新的封禁消息列表，并更新消息服务的封禁列表
    redisContext *cache = g_RedisPoolManager.getCoreCache()->take();
    if (!cache) {
        ERROR_LOG("GatewayServer::banMsgHandle -- connect to cache failed\n");
        return;
    }

    std::string banMsgData;
    switch (RedisUtils::GetBanMsgData(cache, banMsgData)) {
        case REDIS_DB_ERROR: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GatewayServer::banMsgHandle -- get ban msg data failed for db error");
            return;
        }
        case REDIS_FAIL: {
            g_RedisPoolManager.getCoreCache()->put(cache, true);
            ERROR_LOG("GatewayServer::banMsgHandle -- get ban msg data failed for invalid data type\n");
            return;
        }
    }
    g_RedisPoolManager.getCoreCache()->put(cache, false);

    Document doc;
    if (doc.Parse(banMsgData.c_str()).HasParseError()) {
        ERROR_LOG("GatewayServer::banMsgHandle -- parse ban msg data failed\n");
        return;
    }

    if (!doc.IsArray()) {
        ERROR_LOG("GatewayServer::banMsgHandle -- parse ban msg data failed for invalid type\n");
        return;
    }

    std::map<int, bool> banMsgMap;
    std::list<int> banMsgList;
    for (SizeType i = 0; i < doc.Size(); i++) {
        int msgType = doc[i].GetInt();
        if (banMsgMap.find(msgType) == banMsgMap.end()) { // 防重
            banMsgList.push_back(msgType);
            banMsgMap.insert(std::make_pair(msgType, true));
        }
    }
    if (banMsgList.size() > 0) {
        msgServer->setBanMessages(banMsgList);
    }
}
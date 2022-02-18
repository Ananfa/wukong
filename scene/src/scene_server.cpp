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
#include "gateway_client.h"
#include "record_client.h"
#include "scene_client.h"

#include "zk_client.h"
#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

void SceneServer::enterZoo() {
    g_ZkClient.init(g_SceneConfig.getZookeeper(), ZK_TIMEOUT, []() {
        // 对servers配置中每一个server进行节点注册
        g_ZkClient.createEphemeralNode(g_SceneConfig.getZooPath(), ZK_DEFAULT_VALUE, [](const std::string &path, const ZkRet &ret) {
            if (ret) {
                LOG("create rpc node:[%s] sucessful\n", path.c_str());
            } else {
                ERROR_LOG("create rpc node:[%d] failed, code = %d\n", path.c_str(), ret.code());
            }
        });

        g_ZkClient.watchChildren(ZK_GATEWAY_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
            std::vector<GatewayClient::AddressInfo> addresses;
            addresses.reserve(values.size());
            for (const std::string &value : values) {
                GatewayClient::AddressInfo address;

                if (GatewayClient::parseAddress(value, address)) {
                    addresses.push_back(std::move(address));
                } else {
                    ERROR_LOG("zkclient parse gateway server address error, info = %s\n", value.c_str());
                }
            }
            g_GatewayClient.setServers(addresses);
        });

        g_ZkClient.watchChildren(ZK_RECORD_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
            std::vector<RecordClient::AddressInfo> addresses;
            for (const std::string &value : values) {
                RecordClient::AddressInfo address;

                if (RecordClient::parseAddress(value, address)) {
                    addresses.push_back(std::move(address));
                } else {
                    ERROR_LOG("zkclient parse record server address error, info = %s\n", value.c_str());
                }
            }
            g_RecordClient.setServers(addresses);
        });

        // 若角色可以返回大厅则需要与大厅服建立连接
        if (g_SceneConfig.enableLobbyClient()) {
            g_ZkClient.watchChildren(ZK_LOBBY_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
                std::vector<LobbyClient::AddressInfo> addresses;
                for (const std::string &value : values) {
                    LobbyClient::AddressInfo address;

                    if (LobbyClient::parseAddress(value, address)) {
                        addresses.push_back(std::move(address));
                    } else {
                        ERROR_LOG("zkclient parse record server address error, info = %s\n", value.c_str());
                    }
                }
                g_LobbyClient.setServers(addresses);
            });
        }

        // 若需要场景切换，则需要与其他场景服建立连接
        if (g_SceneConfig.enableSceneClient()) {
            g_ZkClient.watchChildren(ZK_SCENE_SERVER, [](const std::string &path, const std::vector<std::string> &values) {
                std::vector<SceneClient::AddressInfo> addresses;
                for (const std::string &value : values) {
                    SceneClient::AddressInfo address;

                    if (SceneClient::parseAddress(value, address)) {
                        // 注意：不需要跳过自身，本地场景服的访问也需要通过SceneClient
                        addresses.push_back(std::move(address));
                    } else {
                        ERROR_LOG("zkclient parse scene server address error, info = %s\n", value.c_str());
                    }
                }
                g_SceneClient.setServers(addresses);
            });
        }
    });
}

void SceneServer::lobbyThread(InnerRpcServer *server, ServerId sid) {
    // 启动RPC服务
    server->start(false);
    
    GameObjectManager *mgr = new GameObjectManager(GAME_SERVER_TYPE_SCENE, sid);
    mgr->init();

    InnerSceneServiceImpl *sceneServiceImpl = new InnerSceneServiceImpl(mgr);
    InnerGameServiceImpl *gameServiceImpl = new InnerGameServiceImpl(mgr);
    server->registerService(sceneServiceImpl);
    server->registerService(gameServiceImpl);

    RoutineEnvironment::runEventLoop();
}

bool SceneServer::init(int argc, char * argv[]) {
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
    if (!g_SceneConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    _io = IO::create(g_SceneConfig.getIoRecvThreadNum(), g_SceneConfig.getIoSendThreadNum());

    // 初始化rpc clients
    _rpcClient = RpcClient::create(_io);

    return true;
}

void SceneServer::run() {
    SceneServiceImpl *sceneServiceImpl = new SceneServiceImpl();
    GameServiceImpl *gameServiceImpl = new GameServiceImpl();

    // 根据servers配置启动Lobby服务，每线程跑一个服务
    const std::vector<SceneConfig::ServerInfo> &sceneInfos = g_SceneConfig.getServerInfos();
    for (auto &info : sceneInfos) {
        // 创建内部RPC服务
        InnerRpcServer *innerServer = new InnerRpcServer();

        sceneServiceImpl->addInnerStub(info.id, new pb::InnerSceneService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));
        gameServiceImpl->addInnerStub(info.id, new pb::InnerGameService_Stub(new InnerRpcChannel(innerServer), ::google::protobuf::Service::STUB_OWNS_CHANNEL));

        _threads.push_back(std::thread(lobbyThread, innerServer, info.id));
    }

    // 启动对外的RPC服务
    RpcServer *server = RpcServer::create(_io, 0, g_SceneConfig.getIp(), g_SceneConfig.getPort());
    server->registerService(sceneServiceImpl);
    server->registerService(gameServiceImpl);

    enterZoo();
    RoutineEnvironment::runEventLoop();
}
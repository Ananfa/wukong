/*
 * Created by Xianke Liu on 2025/12/26.
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

#include "battle_server.h"
#include "corpc_routine_env.h"
#include "corpc_rpc_client.h"
#include "corpc_rpc_server.h"
#include "corpc_message_server.h"

#include "battle_config.h"
#include "battle_room_types_table.h"
#include "design_config/design_config_hub.h"
#include "design_config/design_json_io.h"
#include "battle_service.h"
#include "battle_role_manager.h"
#include "battle_handler.h"
#include "battle_room_manager.h"
#include "agent_manager.h"
#include "lobby_agent.h"
#include "rapidjson/document.h"

#include "utility.h"
#include "share/const.h"

#include <signal.h>
#include <sys/stat.h>

using namespace rapidjson;
using namespace corpc;
using namespace wukong;

bool BattleServer::init(int argc, char * argv[]) {
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
    if (!g_BattleConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }

    // 注册并加载策划表（BattleRoomTypes 等）；未加载时 g_BattleRoomTypesTable 为空指针会在开房 RPC 中崩溃
    registerBattleDesignTables();
    {
        std::string manifestPath = design_config::resolveDataPath(
            configFileName, g_BattleConfig.getDesignConfigManifest().c_str());
        if (manifestPath.empty() || !g_DesignConfigHub.loadFromManifest(manifestPath.c_str())) {
            ERROR_LOG("BattleServer::init -- load design config failed: %s\n",
                      manifestPath.empty() ? "(empty path)" : manifestPath.c_str());
            return false;
        }
        LOG("BattleServer::init -- design config loaded from %s\n", manifestPath.c_str());
    }
    
    // create IO layer
    io_ = IO::create(g_BattleConfig.getIoRecvThreadNum(), g_BattleConfig.getIoSendThreadNum(), 0);

    // 初始化rpc clients
    rpcClient_ = RpcClient::create(io_);

    g_AgentManager.registerAgent(new LobbyAgent(rpcClient_));

    pb::ServerInfo serverInfo;
    serverInfo.set_server_type(SERVER_TYPE_BATTLE);
    serverInfo.set_server_id(g_BattleConfig.getId());
    serverInfo.set_rpc_host(g_BattleConfig.getInternalIp());
    serverInfo.set_rpc_port(g_BattleConfig.getRpcPort());
    auto battleInfo = serverInfo.mutable_battle_info();
    battleInfo->set_msg_host(g_BattleConfig.getExternalIp());
    battleInfo->set_msg_port(g_BattleConfig.getMsgPort());
    if (!g_AgentManager.init(io_, g_BattleConfig.getNexusAddr().host, g_BattleConfig.getNexusAddr().port, serverInfo)) {
        ERROR_LOG("agent manager init failed\n");
        return false;
    }

    g_BattleRoleManager.init();
    g_BattleRoomManager.init();

    return true;
}

void BattleServer::run() {
    // 帧同步战斗流程（框架层）：
    // Lobby requestBattleAssignment：按 mode+battle_def_id+阵营空位(humanCount)匹配房间或新建；
    // 策划表 slotsPerFaction / joinWindowSec / battleDurationSec；分配时占席，不查 GameCore。
    // KCP AUTH 后 StartGameAIOnly + PossessAITank；掉线/未鉴权超时踢出并释放席位；空房超时销毁。

    // 启动对外的RPC服务
    RpcServer *server = RpcServer::create(io_, nullptr, g_BattleConfig.getInternalIp(), g_BattleConfig.getRpcPort());
    server->registerService(new BattleServiceImpl());

    corpc::KcpMessageTerminal *terminal = new corpc::KcpMessageTerminal(true, true, true, true);
    BattleHandler::registerMessages(terminal);

    corpc::KcpMessageServer *msgServer = new corpc::KcpMessageServer(io_, nullptr, terminal, "0.0.0.0", g_BattleConfig.getMsgPort());
    msgServer->start();

    g_AgentManager.start();

    RoutineEnvironment::runEventLoop();
}

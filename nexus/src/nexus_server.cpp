/*
 * Created by Xianke Liu on 2024/7/5.
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

#include "nexus_server.h"

#include "corpc_routine_env.h"
#include "nexus_config.h"
#include "nexus_handler.h"
#include "utility.h"

#include <signal.h>
#include <sys/stat.h>

using namespace corpc;
using namespace wukong;

bool NexusServer::init(int argc, char * argv[]) {
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
    if (!g_NexusConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    return true;
}

void NexusServer::run() {
    IO *io = IO::create(g_NexusConfig.getIoRecvThreadNum(), g_NexusConfig.getIoSendThreadNum(), 0);

    corpc::MessageTerminal *terminal = new corpc::MessageTerminal(true, true, true, true);
    
    NexusHandler::registerMessages(terminal);

    corpc::TcpMessageServer *server = new corpc::TcpMessageServer(io, nullptr, terminal, g_NexusConfig.getIp(), g_NexusConfig.getPort());
    server->start();

    RoutineEnvironment::runEventLoop();
}
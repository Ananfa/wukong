/*
 * Created by Xianke Liu on 2024/6/15.
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

#include "front_server.h"

#include "corpc_routine_env.h"

#include "front_config.h"
#include "agent_manager.h"
#include "gateway_agent.h"

#include "redis_pool.h"
#include "redis_utils.h"

#include "utility.h"
#include "share/const.h"

#include "game.pb.h"

#include <signal.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>

using namespace corpc;
using namespace wukong;

#define MAX_AUTH_MESSAGE_SIZE 0x80

ThrotleController::ThrotleController(): lastWinIndex_(0), lastTimestamp_(0) {
    for (int i = 0; i < 8; i++) {
        windows_[i] = 0;
    }
}

uint32_t ThrotleController::addNum(int64_t ts, uint32_t num) {
    if (ts <= lastTimestamp_) {
        windows_[lastWinIndex_] += num;
    } else if (ts - lastTimestamp_ >= 8) {
        for (int i = 0; i < 8; i++) {
            windows_[i] = 0;
        }

        lastTimestamp_ = ts;
        lastWinIndex_ = 0;
        windows_[lastWinIndex_] = num;
    } else {
        int i = (lastWinIndex_ + 1) & 7;
        lastWinIndex_ = (lastWinIndex_ + ts - lastTimestamp_) & 7;
        while (i != lastWinIndex_) {
            windows_[i] = 0;
            i = (i + 1) & 7;
        }

        windows_[lastWinIndex_] = num;
    }

    return windows_[lastWinIndex_];
}

uint32_t ThrotleController::getAvg() {
    uint32_t total = 0;
    for (int i = 0; i < 8; i++) {
        total += windows_[i];
    }

    return total >> 3;
}

bool FrontServer::init(int argc, char * argv[]) {
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
    if (!g_FrontConfig.parse(configFileName)) {
        ERROR_LOG("Parse config error\n");
        return false;
    }
    
    // create IO layer
    io_ = IO::create(1, 1, 0);

    g_AgentManager.registerAgent(new GatewayAgent(nullptr));

    pb::ServerInfo serverInfo;
    serverInfo.set_server_type(SERVER_TYPE_FRONT);
    serverInfo.set_server_id(g_FrontConfig.getId());
    if (!g_AgentManager.init(io_, g_FrontConfig.getNexusAddr().host, g_FrontConfig.getNexusAddr().port, serverInfo)) {
        ERROR_LOG("agent manager init failed\n");
        return false;
    }

    // 数据库初始化
    const std::vector<RedisInfo>& redisInfos = g_FrontConfig.getRedisInfos();
    for (auto &info : redisInfos) {
        RedisPool *pool = RedisPool::create(info.host.c_str(), info.pwd.c_str(), info.port, info.dbIndex, info.maxConnect);
        if (!g_RedisPoolManager.addPool(info.dbName, pool)) {
            ERROR_LOG("FrontServer::init -- addPool[%s] failed\n", info.dbName.c_str());
            return false;
        }
    }

    g_RedisPoolManager.setCoreCache(g_FrontConfig.getCoreCache());

    ip_ = g_FrontConfig.getIp();
    port_ = g_FrontConfig.getPort();

    if (port_ == 0) {
        ERROR_LOG("FrontServer::init -- port can't be 0\n");
        return false;
    }
    
    socklen_t addrlen = sizeof(localAddr_);
    bzero(&localAddr_, addrlen);
    localAddr_.sin_family = AF_INET;
    localAddr_.sin_port = htons(g_FrontConfig.getPort());
    int nIP = 0;
    
    if (ip_.empty() ||
        ip_.compare("0") == 0 ||
        ip_.compare("0.0.0.0") == 0 ||
        ip_.compare("*") == 0) {
        nIP = htonl(INADDR_ANY);
    }
    else
    {
        nIP = inet_addr(ip_.c_str());
        if (nIP == INADDR_NONE) {
            ERROR_LOG("FrontServer::init -- ip invalid\n");
            return false;
        }
    }
    localAddr_.sin_addr.s_addr = nIP;

    if (g_FrontConfig.getInflowThreadNum() == 0) {
        ERROR_LOG("FrontServer::init -- inflowThreadNum config cant be zero\n");
        return false;
    }

    if (g_FrontConfig.getOutflowThreadNum() == 0) {
        ERROR_LOG("FrontServer::init -- outflowThreadNum config cant be zero\n");
        return false;
    }

    return true;
}

void FrontServer::run() {
    uint32_t inflowThreadNum = g_FrontConfig.getInflowThreadNum();
    uint32_t outflowThreadNum = g_FrontConfig.getOutflowThreadNum();

    threads_.reserve(inflowThreadNum + outflowThreadNum);

    for (int i = 0; i < inflowThreadNum; ++i) {
        threads_.push_back(std::thread(inflowThreadEntry));
    }

    for (int i = 0; i < outflowThreadNum; ++i) {
        threads_.push_back(std::thread(outflowThreadEntry));
    }

    listenFd_ = socket(AF_INET,SOCK_STREAM, IPPROTO_TCP);
    if( listenFd_ >= 0 )
    {
        int nReuseAddr = 1;
        setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &nReuseAddr, sizeof(nReuseAddr));
        
        if( bind(listenFd_, (struct sockaddr*)&localAddr_, sizeof(localAddr_)) == -1 )
        {
            close(listenFd_);
            listenFd_ = -1;
        }
    }
    
    if(listenFd_==-1){
        ERROR_LOG("FrontServer::run -- Can't create socket on %s:%d\n", ip_.c_str(), port_);
        return;
    }

    RoutineEnvironment::startCoroutine(acceptRoutine, nullptr);
    
    g_AgentManager.start();

    RoutineEnvironment::runEventLoop();
}

void FrontServer::inflowThreadEntry( ) {
    RoutineEnvironment::startCoroutine(inflowQueueConsumeRoutine, nullptr);

    RoutineEnvironment::runEventLoop();
}

void FrontServer::outflowThreadEntry( ) {
    RoutineEnvironment::startCoroutine(outflowQueueConsumeRoutine, nullptr);

    RoutineEnvironment::runEventLoop();
}

void *FrontServer::acceptRoutine( void * arg ) {
    int listenFd = g_FrontServer.listenFd_;
    
    LOG("start listen %d %s:%d\n", listenFd, g_FrontServer.ip_.c_str(), g_FrontServer.port_);
    listen( listenFd, 1024 );
    
    // 注意：由于accept方法没有进行hook，只好将它设置为NONBLOCK并且自己对它进行poll
    int iFlags = fcntl(listenFd, F_GETFL, 0);
    iFlags |= O_NONBLOCK;
    iFlags |= O_NDELAY;
    fcntl(listenFd, F_SETFL, iFlags);
    
    // 侦听连接，并把接受的连接传给连接处理对象
    while (true) {
        sockaddr_in addr; //maybe sockaddr_un;
        memset( &addr,0,sizeof(addr) );
        socklen_t len = sizeof(addr);
        
        int fd = co_accept(listenFd, (struct sockaddr *)&addr, &len);
        if( fd < 0 )
        {
            struct pollfd pf = { 0 };
            pf.fd = listenFd;
            pf.events = (POLLIN|POLLERR|POLLHUP);
            co_poll( co_get_epoll_ct(),&pf,1,10000 );
            continue;
        }
        
        LOG("accept fd %d\n", fd);
        // 保持连接
        setKeepAlive(fd, 10);
        
        // 设置读写超时时间，默认为1秒
        co_set_timeout(fd, -1, 1000);
        
        //self->queue_.push(fd);
        RoutineEnvironment::startCoroutine(authRoutine, (void *)fd);
    }
    
    return NULL;
}

//void *FrontServer::queueConsumeRoutine( void * arg ) {
//    FrontServer *self = (FrontServer *)arg;
//
//    while (true) {
//        int fd = self->queue_.pop();
//
//        RoutineEnvironment::startCoroutine(authRoutine, (void *)fd);
//    }
//
//    return NULL;
//}

void *FrontServer::authRoutine( void * arg ) {
    int fd = (uint64_t)arg;

    std::string buffs(MAX_AUTH_MESSAGE_SIZE,0);
    uint8_t *buf = (uint8_t *)buffs.data();
    int retryTimes = 0;
    int recvNum = 0;
    uint32_t bodySize;
    while (true) {
        // 先将数据读到缓存中（尽可能多的读）
        int ret = (int)read(fd, buf + recvNum, MAX_AUTH_MESSAGE_SIZE - recvNum);
        
        if (ret <= 0) {
            // ret 0 mean disconnected
            if (ret < 0 && errno == EAGAIN) {
                // 这里设置最大重试次数
                if (retryTimes < 5) {
                    msleep(100);
                    retryTimes++;
                    continue;
                }
            }
            
            // 出错处理（断线）
            DEBUG_LOG("FrontServer::authRoutine -- read reqhead fd %d ret %d errno %d (%s)\n",
                   fd, ret, errno, strerror(errno));
            
            close(fd);
            return NULL;
        }

        recvNum += ret;

        if (recvNum < CORPC_MESSAGE_HEAD_SIZE) {
            continue;
        }

        bodySize = *(uint32_t*)buf;
        bodySize = be32toh(bodySize);
        int16_t msgType = *(int16_t *)(buf + 4);
        msgType = be16toh(msgType);

        if (msgType != C2S_MESSAGE_ID_AUTH) {
            ERROR_LOG("FrontServer::authRoutine -- not auth message, %d\n", msgType);
            
            close(fd);
            return NULL;
        }

        if (bodySize > MAX_AUTH_MESSAGE_SIZE - CORPC_MESSAGE_HEAD_SIZE) { // 数据超长
            ERROR_LOG("FrontServer::authRoutine -- message too large, %d > %d\n", bodySize, MAX_AUTH_MESSAGE_SIZE - CORPC_MESSAGE_HEAD_SIZE);
            
            close(fd);
            return NULL;
        }

        if (recvNum - CORPC_MESSAGE_HEAD_SIZE < bodySize) {
            continue;
        }

        break;
    }

    // 解析第一个消息（AuthRequest消息）
    pb::AuthRequest authMsg;
    if (!authMsg.ParseFromArray(buf + CORPC_MESSAGE_HEAD_SIZE, bodySize)) {
        ERROR_LOG("FrontServer::authRoutine -- parse auth message failed\n");

        close(fd);
        return NULL;
    }

    // 获得gateid对应gate服信息
    GatewayAgent *gateAgent = (GatewayAgent*)g_AgentManager.getAgent(SERVER_TYPE_GATE);
    pb::ServerInfo *gateSvrInfo = gateAgent->getServerInfo(authMsg.gateid());
    if (!gateSvrInfo) {
        ERROR_LOG("FrontServer::authRoutine -- gate[%d] not found\n", authMsg.gateid());

        close(fd);
        return NULL;
    }

    // 连接gate服
    int gateFd;
    if ((gateFd=socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        ERROR_LOG("FrontServer::authRoutine -- can't create gate connect socket\n");

        close(fd);
        return NULL;
    }
    co_set_timeout(gateFd, -1, 1000);
    
    struct sockaddr_in addr;
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(gateSvrInfo->gate_info().msg_port());
    int nIP = inet_addr(gateSvrInfo->gate_info().msg_host().c_str());
    addr.sin_addr.s_addr = nIP;
    
    if (connect(gateFd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        ERROR_LOG("FrontServer::authRoutine -- can't connect to gate\n");

        close(gateFd);
        close(fd);
        return false;
    }
    setKeepAlive(gateFd, 10);

    // 将先前收到的原始数据发给gate服
    int ret;
    uint32_t sentNum = 0;
    uint32_t leftNum = recvNum;
    do {
        ret = (int)::write(gateFd, buf + sentNum, leftNum);
        if (ret > 0) {
            sentNum += ret;
            leftNum -= ret;
        }
    } while (leftNum > 0 && (errno == EAGAIN || errno == EINTR));

    if (leftNum > 0) {
        WARN_LOG("FrontServer::authRoutine -- write to gate failed, ret %d errno %d (%s)\n",
                   ret, errno, strerror(errno));

        close(gateFd);
        close(fd);
        return false;
    }

    std::shared_ptr<TransportConnection> tCon(new TransportConnection());
    tCon->clientFd = fd;
    tCon->gateFd = gateFd;

    // 创建流入和流出两协程（参数是转发对象的共享指针）
    // 注意：inflowRoutine和outFlowRoutine不能在同一线程中运行
    g_FrontServer.inflowQueue_.push(tCon);
    g_FrontServer.outflowQueue_.push(tCon);

    //RoutineEnvironment::startCoroutine(inflowRoutine, tCon);
    //RoutineEnvironment::startCoroutine(outflowRoutine, tCon);

    return NULL;
}

void *FrontServer::inflowQueueConsumeRoutine( void * arg ) {
    while (true) {
        std::shared_ptr<TransportConnection> conn = g_FrontServer.inflowQueue_.pop();

        RoutineEnvironment::startCoroutine(inflowRoutine, conn.get());
    }

    return NULL;
}

void *FrontServer::outflowQueueConsumeRoutine( void * arg ) {
    while (true) {
        std::shared_ptr<TransportConnection> conn = g_FrontServer.outflowQueue_.pop();

        RoutineEnvironment::startCoroutine(outflowRoutine, conn.get());
    }

    return NULL;
}

void *FrontServer::inflowRoutine( void * arg ) {
    std::shared_ptr<TransportConnection> tCon = ((TransportConnection *)arg)->getPtr();

    std::unique_ptr<ThrotleController> throtle;
    if (g_FrontConfig.getThrotle().open) {
        throtle.reset(new ThrotleController());
    }

    std::string buffs(CORPC_MAX_BUFFER_SIZE,0);
    uint8_t *buf = (uint8_t *)buffs.data();
    int retryTimes = 0;
    while (true) {
        int ret = (int)read(tCon->clientFd, buf, CORPC_MAX_BUFFER_SIZE);
        
        if (ret <= 0) {
            // ret 0 mean disconnected
            if (ret < 0 && errno == EAGAIN) {
                // 这里设置最大重试次数
                if (retryTimes < 5) {
                    msleep(100);
                    retryTimes++;
                    continue;
                }
            }
            
            // 出错处理（断线）
            DEBUG_LOG("FrontServer::inflowRoutine -- read from client fd %d ret %d errno %d (%s)\n",
                   tCon->clientFd, ret, errno, strerror(errno));
            
            break;
        }

        // 限流处理
        if (g_FrontConfig.getThrotle().open) {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            if (throtle->addNum(tv.tv_sec, ret) >= g_FrontConfig.getThrotle().max) {
                ERROR_LOG("FrontServer::inflowRoutine -- client fd %d overflow max\n",
                       tCon->clientFd);
                
                break;
            }

            if (throtle->getAvg() >= g_FrontConfig.getThrotle().avg) {
                ERROR_LOG("FrontServer::inflowRoutine -- client fd %d overflow avg\n",
                       tCon->clientFd);
                
                break;
            }
        }

        uint32_t sentNum = 0;
        uint32_t leftNum = ret;
        do {
            ret = (int)::write(tCon->gateFd, buf + sentNum, leftNum);
            if (ret > 0) {
                sentNum += ret;
                leftNum -= ret;
            }
        } while (leftNum > 0 && (errno == EAGAIN || errno == EINTR));

        if (leftNum > 0) {
            DEBUG_LOG("FrontServer::inflowRoutine -- write to gate fd %d ret %d errno %d (%s)\n",
                   tCon->gateFd, ret, errno, strerror(errno));

            break;
        }
    }

    shutdown(tCon->clientFd, SHUT_WR);
    shutdown(tCon->gateFd, SHUT_RD);

    tCon->closeSem.wait();

    close(tCon->clientFd);
    close(tCon->gateFd);

    return NULL;
}

void *FrontServer::outflowRoutine( void * arg ) {
    std::shared_ptr<TransportConnection> tCon = ((TransportConnection *)arg)->getPtr();

    std::string buffs(CORPC_MAX_BUFFER_SIZE,0);
    uint8_t *buf = (uint8_t *)buffs.data();
    int retryTimes = 0;
    while (true) {
        int ret = (int)read(tCon->gateFd, buf, CORPC_MAX_BUFFER_SIZE);
        
        if (ret <= 0) {
            // ret 0 mean disconnected
            if (ret < 0 && errno == EAGAIN) {
                // 这里设置最大重试次数
                if (retryTimes < 5) {
                    msleep(100);
                    retryTimes++;
                    continue;
                }
            }
            
            // 出错处理（断线）
            DEBUG_LOG("FrontServer::outflowRoutine -- read from gate fd %d ret %d errno %d (%s)\n",
                   tCon->gateFd, ret, errno, strerror(errno));
            
            break;
        }

        uint32_t sentNum = 0;
        uint32_t leftNum = ret;
        do {
            ret = (int)::write(tCon->clientFd, buf + sentNum, leftNum);
            if (ret > 0) {
                sentNum += ret;
                leftNum -= ret;
            }
        } while (leftNum > 0 && (errno == EAGAIN || errno == EINTR));

        if (leftNum > 0) {
            DEBUG_LOG("FrontServer::outflowRoutine -- write to client fd %d ret %d errno %d (%s)\n",
                   tCon->clientFd, ret, errno, strerror(errno));

            break;
        }
    }

    shutdown(tCon->clientFd, SHUT_RD);
    shutdown(tCon->gateFd, SHUT_WR);

    tCon->closeSem.post();

    return NULL;
}


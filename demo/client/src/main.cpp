#include "corpc_routine_env.h"
#include "corpc_message_client.h"
#include "http_client.h"
#include "uuid_utils.h"
#include "share/const.h"
#include "demo_const.h"
#include "rapidjson/document.h"
#include "game.pb.h"
#include "common.pb.h"
#include <iostream>
#include <ctime>
#include <thread>

//#define TEST_RECONNECT

uint32_t numPerThread = 20;
bool reloginWithSameAccount = false;

uint32_t maxNum = 200000;
std::atomic<int> g_count(0);
std::atomic<uint64_t> g_totalCostTm(0);
std::atomic<uint64_t> g_maxCostTm(0);
std::atomic<int> g_cnt(0);


using namespace demo;
using namespace corpc;
using namespace wukong;
using namespace rapidjson;

struct AccountInfo {
    uint32_t account;
    std::string cipher;
    uint32_t userId = 0;
    uint32_t roleId = 0;
    std::string gToken;
    uint32_t lastRecvSerial = 0;
    uint32_t gateId = 0;
    std::string gatewayHost;
    uint16_t gatewayPort;
};

static void *log_routine( void *arg )
{
    //co_enable_hook_sys();
    
    int total = 0;
    int average = 0;
    
    time_t startAt = time(NULL);
    
    while (true) {
        sleep(1);
        
        int cnt = g_cnt.load();
        total += cnt;
        
        if (total == 0) {
            startAt = time(NULL);
            continue;
        }
        
        time_t now = time(NULL);
        
        time_t difTime = now - startAt;
        if (difTime > 0) {
            average = total / difTime;
        } else {
            average = total;
        }

        uint64_t totalCostTm = g_totalCostTm.load();
        int count = g_count.load();
        uint64_t averageCostTm = totalCostTm / count;
        uint64_t maxCostTm = g_maxCostTm.load();
        
        LOG("time %ld seconds, cnt: %d, average: %d, total: %d, averageCost: %d, maxCost: %d\n", difTime, cnt, average, total, averageCostTm, maxCostTm);
        
        g_cnt -= cnt;
    }
    
    return NULL;
}

static void *test_login(void *arg) {
    AccountInfo *accountInfo = (AccountInfo *)arg;
    DEBUG_LOG("account: %d\n", accountInfo->account);
    std::string account = std::to_string(accountInfo->account);
    struct timeval loginStartTm;
    gettimeofday(&loginStartTm, NULL);

    if (accountInfo->userId == 0) {
        std::string token;

        // ====== 登录 ===== //
        {
            DEBUG_LOG("start login account: %s\n", account.c_str());
            HttpRequest request;
            request.setTimeout(5000);
            request.setUrl("http://127.0.0.1:11000/login");
            request.setQueryHeader("Content-Type", "application/x-www-form-urlencoded");
            request.addQueryParam("openid", account.c_str());
            g_HttpClient.doPost(&request, [&](const HttpResponse &response) {
                DEBUG_LOG("login response: %s\n", response.body().c_str());
                Document doc;
                if (doc.Parse(response.body().c_str()).HasParseError()) {
                    ERROR_LOG("login failed for parse response error\n");
                    return;
                }

                if (!doc.HasMember("retCode")) {
                    ERROR_LOG("login failed -- retCode not define\n");
                    return;
                }

                int retCode = doc["retCode"].GetInt();
                if (retCode != 0) {
                    ERROR_LOG("login failed -- %s\n", response.body().c_str());
                    return;
                }

                accountInfo->userId = doc["userId"].GetUint();
                token = doc["token"].GetString();

                const Value& roles = doc["roles"];
                if (roles.Size() > 0) {
                    const Value& role = roles[0];

                    accountInfo->roleId = role["roleId"].GetUint();
                }
            });
        }

        DEBUG_LOG("userId: %d, roleId: %d, token: %s\n", accountInfo->userId, accountInfo->roleId, token.c_str());
        if (accountInfo->userId == 0) {
            ERROR_LOG("login failed, account: %s\n", account.c_str());
            return nullptr;
        }

        // ====== 创角 ====== //
        if (accountInfo->roleId == 0) {
            DEBUG_LOG("start create role for account: %s, userId: %d\n", account.c_str(), accountInfo->userId);
            HttpRequest request;
            request.setTimeout(5000);
            request.setUrl("http://127.0.0.1:11000/createRole");
            request.setQueryHeader("Content-Type", "application/x-www-form-urlencoded");
            request.addQueryParam("userId", std::to_string(accountInfo->userId));
            request.addQueryParam("token", token);
            request.addQueryParam("serverId", "1");
            request.addQueryParam("name", "robot");
            g_HttpClient.doPost(&request, [&](const HttpResponse &response) {
                DEBUG_LOG("create role response: %s\n", response.body().c_str());
                Document doc;
                if (doc.Parse(response.body().c_str()).HasParseError()) {
                    ERROR_LOG("create role failed for parse response error\n");
                    return;
                }

                if (!doc.HasMember("retCode")) {
                    ERROR_LOG("create role failed -- retCode not define\n");
                    return;
                }

                int retCode = doc["retCode"].GetInt();
                if (retCode != 0) {
                    ERROR_LOG("create role failed -- %s\n", response.body().c_str());
                    return;
                }

                const Value& roles = doc["role"];
                accountInfo->roleId = roles["roleId"].GetUint();
            });
        }

        if (accountInfo->roleId == 0) {
            ERROR_LOG("create role failed, account: %s, userId: %d\n", account.c_str(), accountInfo->userId);
            return nullptr;
        }

        // ====== 选择角色进游戏 ====== //
        {
            DEBUG_LOG("start enter game for account: %s, userId: %d, roleId: %d\n", account.c_str(), accountInfo->userId, accountInfo->roleId);
            HttpRequest request;
            request.setTimeout(5000);
            request.setUrl("http://127.0.0.1:11000/enterGame");
            request.setQueryHeader("Content-Type", "application/x-www-form-urlencoded");
            request.addQueryParam("userId", std::to_string(accountInfo->userId));
            request.addQueryParam("roleId", std::to_string(accountInfo->roleId));
            request.addQueryParam("token", token);
            request.addQueryParam("serverId", "1");
            g_HttpClient.doPost(&request, [&](const HttpResponse &response) {
                DEBUG_LOG("enter game response: %s\n", response.body().c_str());
                Document doc;
                if (doc.Parse(response.body().c_str()).HasParseError()) {
                    ERROR_LOG("enter game failed for parse response error\n");
                    return;
                }

                if (!doc.HasMember("retCode")) {
                    ERROR_LOG("enter game failed -- retCode not define\n");
                    return;
                }

                int retCode = doc["retCode"].GetInt();
                if (retCode != 0) {
                    ERROR_LOG("enter game failed -- %s\n", response.body().c_str());
                    return;
                }

                accountInfo->gToken = doc["gToken"].GetString();
                accountInfo->gateId = doc["gateId"].GetUint();
                accountInfo->gatewayHost = doc["host"].GetString();
                accountInfo->gatewayPort = doc["port"].GetUint();
            });
        }

        if (accountInfo->gToken.empty()) {
            ERROR_LOG("enter game failed, account: %s, userId: %d, roleId: %d\n", account.c_str(), accountInfo->userId, accountInfo->roleId);
            return nullptr;
        }

        accountInfo->cipher = UUIDUtils::genUUID();
    }

    // 连接gateway，并发校验身份消息（收到包含角色完整数据的进入游戏消息）
    std::shared_ptr<Crypter> crypter = std::shared_ptr<Crypter>(new SimpleXORCrypter(accountInfo->cipher));
    std::shared_ptr<TcpClient> client(new TcpClient(accountInfo->gatewayHost, accountInfo->gatewayPort, true, true, true, true, crypter, accountInfo->lastRecvSerial));
    client->registerMessage(S2C_MESSAGE_ID_ENTERGAME, std::shared_ptr<google::protobuf::Message>(new pb::DataFragments));
    client->registerMessage(S2C_MESSAGE_ID_RECONNECTED, nullptr);
    client->registerMessage(S2C_MESSAGE_ID_ECHO, std::shared_ptr<google::protobuf::Message>(new pb::StringValue));
    client->registerMessage(S2C_MESSAGE_ID_ENTERLOBBY, std::shared_ptr<google::protobuf::Message>(new pb::Int32Value));


    client->start();

    // 发送身份认证消息
    std::shared_ptr<pb::AuthRequest> authreq(new pb::AuthRequest);
    authreq->set_userid(accountInfo->userId);
    authreq->set_token(accountInfo->gToken.c_str());
    authreq->set_cipher(accountInfo->cipher.c_str());
    authreq->set_recvserial(accountInfo->lastRecvSerial);
    authreq->set_gateid(accountInfo->gateId);
    DEBUG_LOG("======== send auth, account:%s, userId: %d, roleId: %d, recvSerial: %d, gateId: %d\n", account.c_str(), accountInfo->userId, accountInfo->roleId, accountInfo->lastRecvSerial, accountInfo->gateId);
    client->send(C2S_MESSAGE_ID_AUTH, 0, false, authreq);

    // 接收处理从服务器发来
    int16_t rType;
    uint16_t recvTag = 0;
    uint16_t lastRecvTag = 0;
    uint16_t sendTag = 1;
    //uint64_t sendHelloAt = 0;
    bool enterGame = false;
    std::shared_ptr<google::protobuf::Message> rMsg;
    while (true) {
        // TODO: 2. 测试断线重连消息重发
        do {
            client->recv(rType, recvTag, rMsg);
            if (!rType) {
                if (!client->isRunning()) {
                    ERROR_LOG("client->recv connection closed, account:%s, userId: %d, roleId: %d, token: %s, enter: %d\n", account.c_str(), accountInfo->userId, accountInfo->roleId, accountInfo->gToken.c_str(), enterGame);

                    exit(0);
                    // 断线处理，由于服务器在处理connect消息和auth消息时有概率顺序反了导致断线，这里直接进行重登
                    //RoutineEnvironment::startCoroutine(test_login, arg);
                    return nullptr;
                }

                msleep(1);
            }
        } while (!rType);

        lastRecvTag = recvTag;
        switch (rType) {
            case S2C_MESSAGE_ID_ENTERGAME: {
                enterGame = true;
                DEBUG_LOG("enter game\n");
                break;
            }
            case S2C_MESSAGE_ID_RECONNECTED: {
                enterGame = true;
                DEBUG_LOG("reconnect game\n");
                break;
            }
            case S2C_MESSAGE_ID_ENTERLOBBY: {
                //enterGame = true;
                DEBUG_LOG("enter lobby\n");
                continue;
            }
            case S2C_MESSAGE_ID_ECHO: {
                std::shared_ptr<pb::StringValue> resp = std::static_pointer_cast<pb::StringValue>(rMsg);
                DEBUG_LOG("echo: %s\n", resp->value().c_str());

                struct timeval loginFinishTm;
                gettimeofday(&loginFinishTm, NULL);

                uint64_t loginCostTime = (loginFinishTm.tv_sec - loginStartTm.tv_sec) * 1000000 + (loginFinishTm.tv_usec - loginStartTm.tv_usec);
                uint64_t totalCostTm = g_totalCostTm.fetch_add(loginCostTime);

                int count = g_count.fetch_add(1);
                
                uint64_t maxCostTime = g_maxCostTm.load();
                if (loginCostTime > maxCostTime) {
                    g_maxCostTm.store(loginCostTime);
                }

                if (count >= maxNum) {
                    ERROR_LOG("finished\n");
                    client->stop();
                } else {
                    //ERROR_LOG("count:%d\n", count);
#ifdef TEST_RECONNECT
                    // BUG: 下面这句打开后会引发奇怪问题--连接建立中途卡住poll没连接事件。原因是老client协程中操作的fd可能与当前fd相同，导致数据被老client协程接收了
                    // 需要确保老client不会截取新client的数据
                    //client->close();
                    client->stop();
                    accountInfo->lastRecvSerial = client->getLastRecvSerial();
                    RoutineEnvironment::startCoroutine(test_login, (void*)accountInfo);
#else
                    // 这里有个问题：重登时正好gameobj向gatewayobj心跳，在gatewayobj的旧对象销毁新对象重建过程中刚好心跳请求到达，因此返回心跳失败--找不到gatewayobj，
                    // 在心跳RPC结果返回过程中，新gatewayobj创建完成并且通知gameobj连接建立，gameobj通知客户端进入游戏完成，（此处进行心跳失败处理将gateway对象stub清除了），
                    // 客户端发1000消息给服务器，gateway将消息转给lobby中的游戏对象，游戏对象返回1000消息发现gateway对象stub为空
                    
                    AccountInfo *newAccountInfo = new AccountInfo;
                    if (reloginWithSameAccount) {
                        newAccountInfo->account = accountInfo->account;
                    } else {
                        client->stop();
                        newAccountInfo->account = accountInfo->account + numPerThread;
                    }
                    delete accountInfo;

                    RoutineEnvironment::startCoroutine(test_login, (void*)newAccountInfo);
#endif
                }

                
                return nullptr;
            }
        }

        if (enterGame) {
            //ERROR_LOG("rType:%d\n", rType);
            g_cnt++;
            DEBUG_LOG("start send \"hello world\"\n");

            // 发Echo消息
            std::shared_ptr<pb::StringValue> echoReq(new pb::StringValue);
            echoReq->set_value("hello world");

            client->send(C2S_MESSAGE_ID_ECHO, ++sendTag, true, std::static_pointer_cast<google::protobuf::Message>(echoReq));
            
        }

    }

    return nullptr;
}

void clientThread(int base, int num) {
    for (int i = 0; i < num; i++) {
        AccountInfo *accountInfo = new AccountInfo;

        //char *account = new char[10];
        //sprintf(account, "%d", base + i);
        //accountInfo->account = account;
        accountInfo->account = base + i;
        RoutineEnvironment::startCoroutine(test_login, (void*)accountInfo);
    }

    LOG("thread %d running...\n", GetPid());
    
    RoutineEnvironment::runEventLoop();
}

int main(int argc, const char *argv[]) {
    RoutineEnvironment::init();
    LOG("start...\n");

    std::thread t1 = std::thread(clientThread, 20000000, numPerThread);
    std::thread t2 = std::thread(clientThread, 30000000, numPerThread);
    std::thread t3 = std::thread(clientThread, 40000000, numPerThread);
    std::thread t4 = std::thread(clientThread, 50000000, numPerThread);
    std::thread t5 = std::thread(clientThread, 60000000, numPerThread);

    RoutineEnvironment::startCoroutine(log_routine, NULL);

    LOG("running...\n");

    RoutineEnvironment::runEventLoop();
    return 0;
}
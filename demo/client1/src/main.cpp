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

using namespace demo;
using namespace corpc;
using namespace wukong;
using namespace rapidjson;

static std::atomic<int> g_cnt(0);

struct AccountInfo {
    std::string account;
    std::string cipher;
    uint32_t userId = 0;
    uint32_t roleId = 0;
    std::string gToken;
    uint32_t lastRecvSerial = 0;
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
        
        total += g_cnt;
        
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
        
        LOG("time %ld seconds, cnt: %d, average: %d, total: %d\n", difTime, int(g_cnt), average, total);
        
        g_cnt = 0;
    }
    
    return NULL;
}

static void *test_login(void *arg) {
    AccountInfo *accountInfo = (AccountInfo *)arg;
    DEBUG_LOG("account: %s\n", accountInfo->account.c_str());

    if (accountInfo->userId == 0) {
        std::string token;

        // ====== 登录 ===== //
        {
            DEBUG_LOG("start login account: %s\n", accountInfo->account.c_str());
            HttpRequest request;
            request.setTimeout(5000);
            request.setUrl("http://127.0.0.1:11000/login");
            request.setQueryHeader("Content-Type", "application/x-www-form-urlencoded");
            request.addQueryParam("openid", accountInfo->account.c_str());
            g_HttpClient.doPost(&request, [&](const HttpResponse &response) {
                //DEBUG_LOG("login response: %s\n", response.body().c_str());
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
            ERROR_LOG("login failed, account: %s\n", accountInfo->account.c_str());
            return nullptr;
        }

        // ====== 创角 ====== //
        if (accountInfo->roleId == 0) {
            DEBUG_LOG("start create role for account: %s, userId: %d\n", accountInfo->account.c_str(), accountInfo->userId);
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
            ERROR_LOG("create role failed, account: %s, userId: %d\n", accountInfo->account.c_str(), accountInfo->userId);
            return nullptr;
        }

        // ====== 选择角色进游戏 ====== //
        {
            DEBUG_LOG("start enter game for account: %s, userId: %d, roleId: %d\n", accountInfo->account.c_str(), accountInfo->userId, accountInfo->roleId);
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
                accountInfo->gatewayHost = doc["host"].GetString();
                accountInfo->gatewayPort = doc["port"].GetUint();
            });
        }

        if (accountInfo->gToken.empty()) {
            ERROR_LOG("enter game failed, account: %s, userId: %d, roleId: %d\n", accountInfo->account.c_str(), accountInfo->userId, accountInfo->roleId);
            return nullptr;
        }

        accountInfo->cipher = UUIDUtils::genUUID();
    }

    // 连接gateway，并发校验身份消息（收到包含角色完整数据的进入游戏消息）
    std::shared_ptr<Crypter> crypter = std::shared_ptr<Crypter>(new SimpleXORCrypter(accountInfo->cipher));
    std::shared_ptr<TcpClient> client(new TcpClient(accountInfo->gatewayHost, accountInfo->gatewayPort, true, true, true, true, crypter, accountInfo->lastRecvSerial));
    client->registerMessage(S2C_MESSAGE_ID_ENTERGAME, std::shared_ptr<google::protobuf::Message>(new pb::DataFragments));
    client->registerMessage(S2C_MESSAGE_ID_RECONNECTED, nullptr);
    client->registerMessage(S2C_MESSAGE_ID_ERROR, std::shared_ptr<google::protobuf::Message>(new pb::Int32Value));
    client->registerMessage(S2C_MESSAGE_ID_ENTERSCENE, std::shared_ptr<google::protobuf::Message>(new pb::Int32Value));
    client->registerMessage(S2C_MESSAGE_ID_ENTERLOBBY, std::shared_ptr<google::protobuf::Message>(new pb::Int32Value));

    client->start();

    // 发送身份认证消息
    std::shared_ptr<pb::AuthRequest> authreq(new pb::AuthRequest);
    authreq->set_userid(accountInfo->userId);
    authreq->set_token(accountInfo->gToken.c_str());
    authreq->set_cipher(accountInfo->cipher.c_str());
    authreq->set_recvserial(accountInfo->lastRecvSerial);
    DEBUG_LOG("======== send auth, account:%s, userId: %d, roleId: %d, recvSerial: %d\n", accountInfo->account.c_str(), accountInfo->userId, accountInfo->roleId, accountInfo->lastRecvSerial);
    client->send(C2S_MESSAGE_ID_AUTH, 0, false, authreq);

    // 接收处理从服务器发来
    int16_t rType;
    uint16_t recvTag = 0;
    uint16_t lastRecvTag = 0;
    uint16_t sendTag = 1;
    bool enterGame = false;
    std::shared_ptr<google::protobuf::Message> rMsg;
    while (true) {
        // TODO: 2. 测试断线重连消息重发
        do {
            //DEBUG_LOG("begin recv\n");
            client->recv(rType, recvTag, rMsg);
            if (!rType) {
                if (!client->isRunning()) {
                    ERROR_LOG("client->recv connection closed, account:%s, userId: %d, roleId: %d, token: %s, enter: %d\n", accountInfo->account.c_str(), accountInfo->userId, accountInfo->roleId, accountInfo->gToken.c_str(), enterGame);

                    exit(0);
                    // 断线处理，由于服务器在处理connect消息和auth消息时有概率顺序反了导致断线，这里直接进行重登
                    //RoutineEnvironment::startCoroutine(test_login, arg);
                    return nullptr;
                }

                msleep(1);
            }
        } while (!rType);

        DEBUG_LOG("recv %d\n", rType);
        lastRecvTag = recvTag;
        switch (rType) {
            case S2C_MESSAGE_ID_ENTERGAME: {
                //enterGame = true;
                DEBUG_LOG("enter game\n");
                break;
            }
            case S2C_MESSAGE_ID_RECONNECTED: {
                //enterGame = true;
                DEBUG_LOG("reconnect game\n");
                break;
            }
            case S2C_MESSAGE_ID_ERROR: {
                std::shared_ptr<pb::Int32Value> resp = std::static_pointer_cast<pb::Int32Value>(rMsg);
                DEBUG_LOG("recv error: %d\n", resp->value());
                break;
            }
            case S2C_MESSAGE_ID_ENTERLOBBY: {
                enterGame = true;
                DEBUG_LOG("enter lobby\n");

                // 发进入场景消息
                std::shared_ptr<pb::Int32Value> enterSceneReq(new pb::Int32Value);
                enterSceneReq->set_value(1);

                client->send(C2S_MESSAGE_ID_ENTERSCENE, 0, true, std::static_pointer_cast<google::protobuf::Message>(enterSceneReq));
                break;
            }
            case S2C_MESSAGE_ID_ENTERSCENE: {
                g_cnt++;
                std::shared_ptr<pb::Int32Value> resp = std::static_pointer_cast<pb::Int32Value>(rMsg);
                DEBUG_LOG("enter scene: %d\n", resp->value());

                // 这里有个问题：重登时正好gameobj向gatewayobj心跳，在gatewayobj的旧对象销毁新对象重建过程中刚好心跳请求到达，因此返回心跳失败--找不到gatewayobj，
                // 在心跳RPC结果返回过程中，新gatewayobj创建完成并且通知gameobj连接建立，gameobj通知客户端进入游戏完成，（此处进行心跳失败处理将gateway对象stub清除了），
                // 客户端发1000消息给服务器，gateway将消息转给lobby中的游戏对象，游戏对象返回1000消息发现gateway对象stub为空

                AccountInfo *newAccountInfo = new AccountInfo;
                newAccountInfo->account = accountInfo->account;
                delete accountInfo;

                RoutineEnvironment::startCoroutine(test_login, (void*)newAccountInfo);
                
                return nullptr;
            }
        }

    }

    return nullptr;
}

void clientThread(int base, int num) {
    for (int i = 0; i < num; i++) {
        AccountInfo *accountInfo = new AccountInfo;

        char *account = new char[10];
        sprintf(account, "%d", base + i);
        accountInfo->account = account;
        RoutineEnvironment::startCoroutine(test_login, (void*)accountInfo);
    }

    LOG("thread %d running...\n", GetPid());
    
    RoutineEnvironment::runEventLoop();
}

int main(int argc, const char *argv[]) {
    RoutineEnvironment::init();
    LOG("start...\n");

    std::thread t1 = std::thread(clientThread, 20000000, 1);
    //std::thread t2 = std::thread(clientThread, 30000000, 20);
    //std::thread t3 = std::thread(clientThread, 40000000, 20);
    //std::thread t4 = std::thread(clientThread, 50000000, 20);
    //std::thread t5 = std::thread(clientThread, 60000000, 20);

    RoutineEnvironment::startCoroutine(log_routine, NULL);

    LOG("running...\n");

    RoutineEnvironment::runEventLoop();
    return 0;
}
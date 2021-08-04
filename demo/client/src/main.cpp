#include "corpc_routine_env.h"
#include "corpc_message_client.h"
#include "http_client.h"
#include "uuid_utils.h"
#include "share/const.h"
#include "rapidjson/document.h"
#include "game.pb.h"
#include "common.pb.h"
#include <iostream>
#include <ctime>

using namespace corpc;
using namespace wukong;
using namespace rapidjson;

static void *test_login(void *arg) {
    const char *account = (const char *)arg;
    DEBUG_LOG("account: %s\n", account);
    uint32_t userId = 0;
    uint32_t roleId = 0;
    std::string token;

    // ====== 登录 ===== //
    {
        LOG("start login account: %s\n", account);
        HttpRequest request;
        request.setTimeout(5000);
        request.setUrl("http://127.0.0.1:11000/login");
        request.setQueryHeader("Content-Type", "application/x-www-form-urlencoded");
        request.addQueryParam("openid", account);
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

            userId = doc["userId"].GetUint();
            token = doc["token"].GetString();

            const Value& roles = doc["roles"];
            if (roles.Size() > 0) {
                const Value& role = roles[0];

                roleId = role["roleId"].GetUint();
            }
        });
    }

    DEBUG_LOG("userId: %d, roleId: %d, token: %s\n", userId, roleId, token.c_str());
    if (userId == 0) {
        ERROR_LOG("login failed, account: %s\n", account);
        return nullptr;
    }

    // ====== 创角 ====== //
    if (roleId == 0) {
        LOG("start create role for account: %s, userId: %d\n", account, userId);
        HttpRequest request;
        request.setTimeout(5000);
        request.setUrl("http://127.0.0.1:11000/createRole");
        request.setQueryHeader("Content-Type", "application/x-www-form-urlencoded");
        request.addQueryParam("userId", std::to_string(userId));
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
            roleId = roles["roleId"].GetUint();
        });
    }

    if (roleId == 0) {
        ERROR_LOG("create role failed, account: %s, userId: %d\n", account, userId);
        return nullptr;
    }

    // ====== 选择角色进游戏 ====== //
    std::string gToken;
    std::string gatewayHost;
    uint16_t gatewayPort;
    {
        LOG("start enter game for account: %s, userId: %d, roleId: %d\n", account, userId, roleId);
        HttpRequest request;
        request.setTimeout(5000);
        request.setUrl("http://127.0.0.1:11000/enterGame");
        request.setQueryHeader("Content-Type", "application/x-www-form-urlencoded");
        request.addQueryParam("userId", std::to_string(userId));
        request.addQueryParam("roleId", std::to_string(roleId));
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

            gToken = doc["gToken"].GetString();
            gatewayHost = doc["host"].GetString();
            gatewayPort = doc["port"].GetUint();
        });
    }

    if (gToken.empty()) {
        ERROR_LOG("enter game failed, account: %s, userId: %d, roleId: %d\n", account, userId, roleId);
        return nullptr;
    }

    // 连接gateway，并发校验身份消息（收到包含角色完整数据的进入游戏消息）
    std::string cipher = UUIDUtils::genUUID();
    std::shared_ptr<Crypter> crypter = std::shared_ptr<Crypter>(new SimpleXORCrypter(cipher));
    std::shared_ptr<TcpClient> client(new TcpClient(gatewayHost, gatewayPort, true, true, true, true, crypter));
    client->registerMessage(S2C_MESSAGE_ID_ENTERGAME, std::shared_ptr<google::protobuf::Message>(new pb::DataFragments));
    client->registerMessage(1000, std::shared_ptr<google::protobuf::Message>(new pb::StringValue));
    
    client->start();
    client->detach();

    // 发送身份认证消息
    std::shared_ptr<pb::AuthRequest> authreq(new pb::AuthRequest);
    authreq->set_userid(userId);
    authreq->set_token(gToken.c_str());
    authreq->set_cipher(cipher.c_str());
    authreq->set_recvserial(0);

    client->send(C2S_MESSAGE_ID_AUTH, 0, false, authreq);

    // 接收处理从服务器发来
    int16_t rType;
    uint16_t sendTag = 1;
    uint16_t recvTag;
    bool enterGame = false;
    std::shared_ptr<google::protobuf::Message> rMsg;
    while (true) {
        // TODO: 2. 测试断线重连消息重发
        do {
            client->recv(rType, recvTag, rMsg);
            if (!rType) {
                msleep(1);
            }
        } while (!rType);

        LOG("recv msg: %d, recvTag: %d\n", rType, recvTag);
        switch (rType) {
            case S2C_MESSAGE_ID_ENTERGAME: {
                enterGame = true;
                LOG("enter game\n");
                break;
            }
            case 1000: {
                std::shared_ptr<pb::StringValue> resp = std::static_pointer_cast<pb::StringValue>(rMsg);
                LOG("echo: %s\n", resp->value().c_str());
                //break;

                // 重登测试
                RoutineEnvironment::startCoroutine(test_login, arg);
                return nullptr;
            }
        }

        if (enterGame) {
            LOG("start send \"hello world\"\n");
            // 发Echo消息
            std::shared_ptr<pb::StringValue> echoReq(new pb::StringValue);
            echoReq->set_value("hello world");

            client->send(1000, ++sendTag, true, std::static_pointer_cast<google::protobuf::Message>(echoReq));
        }

    }

    return nullptr;
}

int main(int argc, const char *argv[]) {
    co_start_hook();

    for (int i = 0; i < 10; i++) {
        int accountId = 20000000 + i;
        char *account = new char[10];
        sprintf(account,"%d",accountId);
        RoutineEnvironment::startCoroutine(test_login, (void*)account);
    }

    LOG("running...\n");

    RoutineEnvironment::runEventLoop();
    return 0;
}
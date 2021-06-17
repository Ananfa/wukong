#include "corpc_routine_env.h"
#include "http_client.h"
#include "rapidjson/document.h"
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
        return nullptr;
    }

    // ====== 创角 ====== //
    if (roleId == 0) {
        HttpRequest request;
        request.setTimeout(5000);
        request.setUrl("http://127.0.0.1:11000/createRole");
        request.setQueryHeader("Content-Type", "application/json");
        request.addQueryParam("userId", std::to_string(userId));
        request.addQueryParam("token", token);
        request.addQueryParam("serverId", "1");
        request.addQueryParam("name", "robot");
        g_HttpClient.doGet(&request, [&](const HttpResponse &response) {
            DEBUG_LOG("createRole response: %s\n", response.body().c_str());

        });
    }

    return nullptr;
}

int main(int argc, const char *argv[]) {
    co_start_hook();

    RoutineEnvironment::startCoroutine(test_login, (void*)"20001003");

    LOG("running...\n");

    RoutineEnvironment::runEventLoop();
    return 0;
}
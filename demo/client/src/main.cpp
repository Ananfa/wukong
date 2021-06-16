#include "corpc_routine_env.h"
#include "http_client.h"
#include <iostream>
#include <ctime>

using namespace corpc;
using namespace wukong;
//using namespace demo;

static void *test_login(void *arg) {
	while (true) {
		HttpRequest request;
		request.setTimeout(5000);
		request.setUrl("http://127.0.0.1:11000/login");
		//request.setQueryHeader("Content-Type", "application/json");
		g_HttpClient.doGet(&request, [&](const HttpResponse &response) {
			std::cout << response.code() << std::endl;
			std::cout << response.body().c_str() << std::endl;
		});
		sleep(10);
	}

	return nullptr;
}

int main(int argc, const char *argv[]) {
	co_start_hook();

	RoutineEnvironment::startCoroutine(test_login, NULL);

	LOG("running...\n");

	RoutineEnvironment::runEventLoop();
	return 0;
}
#include "handler_mgr.h"

using namespace demo;

void HandlerMgr::init(HttpServer *server) {
    
    // 注册filter
    // 验证request是否合法
    server->registerFilter([](std::shared_ptr<RequestMessage>& request, std::shared_ptr<ResponseMessage>& response) -> bool {
        // TODO: authentication
//        response->setResult(HttpStatusCode::Unauthorized);
//        response->addHeader("WWW-Authenticate", "Basic realm=\"Need Authentication\"");
        return true;
    });

    // 注册handler
    server->registerHandler(POST, "/sendMail", HandlerMgr::sendMail);

    // TODO:注册其他handler
}

void HandlerMgr::sendMail(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response) {
	// TODO: 
	DEBUG_LOG("HandlerMgr::sendMail -- start ... \n");
	
}
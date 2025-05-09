#ifndef handler_mgr_h
#define handler_mgr_h

#include "http_server.h"

using namespace wukong;

namespace demo {
    class HandlerMgr {
    public:
        static void init(HttpServer *server);
        static void sendMail(std::shared_ptr<RequestMessage> &request, std::shared_ptr<ResponseMessage> &response);
    };
}

#endif /* handler_mgr_h */
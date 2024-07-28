#include "corpc_routine_env.h"
#include "http_server.h"

using namespace wukong;

HttpPipeline::HttpPipeline(std::shared_ptr<corpc::Connection> &connection, Worker *worker): Pipeline(connection, worker), parser_(new HttpParser), nparsed_(0) {
    
}

bool HttpPipeline::upflow(uint8_t *buf, int size) {
    std::shared_ptr<HttpConnection> connection = std::static_pointer_cast<HttpConnection>(connection_.lock());
    assert(connection);
    
    // 将数据存入缓存
    buf_.append((char*)buf, size);
    
    uint32_t tmp = nparsed_;
    
    while ( nparsed_ - tmp < size) {
        // 解析数据
        nparsed_ += parser_->append(buf_.c_str()+nparsed_, buf_.length()-nparsed_);
        
        if (!parser_->isCompleted()) {
            break;
        }
        
        // 获得完整的HTTP请求
        RequestMessage * request = parser_->getRequest();
        if ( request != NULL )
        {
            // 交由worker处理请求
            HttpWorkerTask *task = HttpWorkerTask::create();
            task->connection = connection;
            task->request = std::shared_ptr<RequestMessage>(request);
            task->response = std::shared_ptr<ResponseMessage>(new ResponseMessage);
            
            worker_->addTask(task);
        }
        
        parser_->reset();
    }
    
    if (nparsed_ > HTTP_PIPELINE_CLEAN_BUF_THRESHOLD) {
        buf_ = buf_.substr(nparsed_);
        nparsed_ = 0;
    }
    
    return true;
}

bool HttpPipeline::downflow(uint8_t *buf, int space, int &size) {
    std::shared_ptr<HttpConnection> connection = std::static_pointer_cast<HttpConnection>(connection_.lock());
    assert(connection);
    
    size = 0;
    while (connection->getDataSize() > 0 && space - size > 0) {
        std::shared_ptr<ResponseMessage> response = std::static_pointer_cast<ResponseMessage>(connection->getFrontData());
        
        if (connection->responseBuf_.empty()) {
            response->serialize(connection->responseBuf_);
            assert(!connection->responseBuf_.empty());
        }
        
        size_t bufLen = connection->responseBuf_.length();
        
        assert(connection->responseBufSentNum_ < bufLen);
        uint8_t *resBuf = (uint8_t *)connection->responseBuf_.data();
        if (space - size < bufLen - connection->responseBufSentNum_) {
            memcpy(buf + size, resBuf + connection->responseBufSentNum_, space - size);
            connection->responseBufSentNum_ += space - size;
            size = space;
            
            return true;
        } else {
            memcpy(buf + size, resBuf + connection->responseBufSentNum_, bufLen - connection->responseBufSentNum_);
            size += bufLen - connection->responseBufSentNum_;
            
            connection->responseBuf_.clear();
            connection->responseBufSentNum_ = 0;
        }
        
        connection->popFrontData();
    }
    
    return true;
}

std::shared_ptr<Pipeline> HttpPipelineFactory::buildPipeline(std::shared_ptr<corpc::Connection> &connection) {
    return std::shared_ptr<Pipeline>( new HttpPipeline(connection, worker_) );
}

HttpConnection::HttpConnection(int fd, HttpServer* server): corpc::Connection(fd, server->io_, false), server_(server), responseBufSentNum_(0) {
}

HttpConnection::~HttpConnection() {
    DEBUG_LOG("HttpConnection::~HttpConnection -- fd:%d in thread:%d\n", fd_, GetPid());
}

void HttpConnection::onConnect() {
    DEBUG_LOG("HttpConnection::onConnect -- connection fd:%d is connected\n", fd_);
}

void HttpConnection::onClose() {
    DEBUG_LOG("HttpConnection::onClose -- connection fd:%d is closed\n", fd_);
    //std::shared_ptr<corpc::Connection> self = corpc::Connection::shared_from_this();
    //server_->onClose(self);
}

void *HttpWorkerTask::taskCallRoutine( void * arg ) {
    HttpWorkerTask *task = (HttpWorkerTask *)arg;
    
    task->connection->getServer()->handle(task->connection, task->request, task->response);
    
    task->destory();
    
    return NULL;
}

void HttpWorkerTask::doTask() {
    RoutineEnvironment::startCoroutine(taskCallRoutine, this);
}

HttpServer::HttpServer(IO *io, Worker *worker, const std::string& ip, uint16_t port): corpc::Server(io) {
    acceptor_ = new TcpAcceptor(this, ip, port);
    
    if (worker) {
        worker_ = worker;
    } else {
        worker_ = io->getWorker();
    }
    
    pipelineFactory_.reset(new HttpPipelineFactory(worker_));
}

HttpServer::~HttpServer() {}

HttpServer* HttpServer::create(IO *io, Worker *worker, const std::string& ip, uint16_t port) {
    assert(io);
    HttpServer *server = new HttpServer(io, worker, ip, port);

    // TODO: 注册默认filter
    
    server->start();
    return server;
}

corpc::Connection *HttpServer::buildConnection(int fd) {
    return new HttpConnection(fd, this);
}

//void HttpServer::onClose(std::shared_ptr<corpc::Connection>& connection) {
//    LOG("HttpServer::onClose -- connection fd:%d is closed\n", connection->getfd());
//}

void HttpServer::registerFilter(HttpFilter filter) {
    filterVec_.push_back(filter);
}

void HttpServer::registerHandler(HttpMethod method, const std::string& uri, HttpHandler handler) {
    switch (method) {
        case GET: {
            getterMap_.insert(std::make_pair(uri, handler));
            break;
        }
        case POST: {
            posterMap_.insert(std::make_pair(uri, handler));
            break;
        }
        default: {
            ERROR_LOG("HttpServer::registerHandler -- unknown method\n");
            break;
        }
    }
}

void HttpServer::send(std::shared_ptr<HttpConnection>& connection, std::shared_ptr<RequestMessage>& request, std::shared_ptr<ResponseMessage>& response) {
    connection->send(response);
    
    if (!request->isKeepalive()) {
        connection->close();
    }
}

void HttpServer::handle(std::shared_ptr<HttpConnection>& connection, std::shared_ptr<RequestMessage>& request, std::shared_ptr<ResponseMessage>& response) {
    std::map<std::string, HttpHandler>::iterator handlerIt;
    if (strcmp(request->getMethod().c_str(), "GET") == 0) {
        handlerIt = getterMap_.find(request->getURI());

        if (handlerIt == getterMap_.end()) {
            response->setResult(HttpStatusCode::NotFound);
            return send(connection, request, response);
        }
    } else if (strcmp(request->getMethod().c_str(), "POST") == 0) {
        handlerIt = posterMap_.find(request->getURI());

        if (handlerIt == posterMap_.end()) {
            response->setResult(HttpStatusCode::NotFound);
            return send(connection, request, response);
        }
    } else {
        response->setResult(HttpStatusCode::NotFound);
        return send(connection, request, response);
    }

    // 先执行filter, 再执行handler
    if (!filterVec_.empty()) {
        for (auto &filter : filterVec_) {
            if (!filter(request, response)) {
                // 如果filter没有设置HttpStatusCode, 默认HttpStatusCode::BadRequest
                if (!response->getStatusCode()) response->setResult(HttpStatusCode::BadRequest);
                return send(connection, request, response);
            }
        }
    }
    
    handlerIt->second(request, response);
    send(connection, request, response);
}

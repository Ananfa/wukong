#include "corpc_routine_env.h"
#include "http_server.h"

using namespace wukong;

HttpPipeline::HttpPipeline(std::shared_ptr<corpc::Connection> &connection, Worker *worker): Pipeline(connection, worker), _parser(new HttpParser), _nparsed(0) {
    
}

bool HttpPipeline::upflow(uint8_t *buf, int size) {
    std::shared_ptr<HttpConnection> connection = std::static_pointer_cast<HttpConnection>(_connection.lock());
    assert(connection);
    
    // 将数据存入缓存
    _buf.append((char*)buf, size);
    
    uint32_t tmp = _nparsed;
    
    while ( _nparsed - tmp < size) {
        // 解析数据
        _nparsed += _parser->append(_buf.c_str()+_nparsed, _buf.length()-_nparsed);
        
        if (!_parser->isCompleted()) {
            break;
        }
        
        // 获得完整的HTTP请求
        RequestMessage * request = _parser->getRequest();
        if ( request != NULL )
        {
            // 交由worker处理请求
            HttpWorkerTask *task = new HttpWorkerTask;
            task->connection = connection;
            task->request = std::shared_ptr<RequestMessage>(request);
            task->response = std::shared_ptr<ResponseMessage>(new ResponseMessage);
            
            _worker->addMessage(task);
        }
        
        _parser->reset();
    }
    
    if (_nparsed > HTTP_PIPELINE_CLEAN_BUF_THRESHOLD) {
        _buf = _buf.substr(_nparsed);
        _nparsed = 0;
    }
    
    return true;
}

bool HttpPipeline::downflow(uint8_t *buf, int space, int &size) {
    std::shared_ptr<HttpConnection> connection = std::static_pointer_cast<HttpConnection>(_connection.lock());
    assert(connection);
    
    size = 0;
    while (connection->getDataSize() > 0 && space - size > 0) {
        std::shared_ptr<ResponseMessage> response = std::static_pointer_cast<ResponseMessage>(connection->getFrontData());
        
        if (connection->_responseBuf.empty()) {
            response->serialize(connection->_responseBuf);
            assert(!connection->_responseBuf.empty());
        }
        
        size_t bufLen = connection->_responseBuf.length();
        
        assert(connection->_responseBufSentNum < bufLen);
        uint8_t *resBuf = (uint8_t *)connection->_responseBuf.data();
        if (space - size < bufLen - connection->_responseBufSentNum) {
            memcpy(buf + size, resBuf + connection->_responseBufSentNum, space - size);
            connection->_responseBufSentNum += space - size;
            size = space;
            
            return true;
        } else {
            memcpy(buf + size, resBuf + connection->_responseBufSentNum, bufLen - connection->_responseBufSentNum);
            size += bufLen - connection->_responseBufSentNum;
            
            connection->_responseBuf.clear();
            connection->_responseBufSentNum = 0;
        }
        
        connection->popFrontData();
    }
    
    return true;
}

std::shared_ptr<Pipeline> HttpPipelineFactory::buildPipeline(std::shared_ptr<corpc::Connection> &connection) {
    return std::shared_ptr<Pipeline>( new HttpPipeline(connection, _worker) );
}

HttpConnection::HttpConnection(int fd, HttpServer* server): corpc::Connection(fd, server->_io, false), _server(server), _responseBufSentNum(0) {
}

HttpConnection::~HttpConnection() {
    LOG("HttpConnection::~HttpConnection -- fd:%d in thread:%d\n", _fd, GetPid());
}

void HttpConnection::onClose() {
    std::shared_ptr<corpc::Connection> self = corpc::Connection::shared_from_this();
    _server->onClose(self);
}

void *HttpServer::MultiThreadWorker::taskCallRoutine( void * arg ) {
    HttpWorkerTask *task = (HttpWorkerTask *)arg;
    
    task->connection->getServer()->handle(task->connection, task->request, task->response);
    
    delete task;
    
    return NULL;
}

void HttpServer::MultiThreadWorker::handleMessage(void *msg) {
    RoutineEnvironment::startCoroutine(taskCallRoutine, msg);
}

void *HttpServer::CoroutineWorker::taskCallRoutine( void * arg ) {
    HttpWorkerTask *task = (HttpWorkerTask *)arg;
    
    task->connection->getServer()->handle(task->connection, task->request, task->response);
    
    delete task;
    
    return NULL;
}

void HttpServer::CoroutineWorker::handleMessage(void *msg) {
    RoutineEnvironment::startCoroutine(taskCallRoutine, msg);
}


HttpServer::HttpServer(IO *io, uint16_t workThreadNum, const std::string& ip, uint16_t port): corpc::Server(io) {
    _acceptor = new TcpAcceptor(this, ip, port);
    
    // 根据需要创建多线程worker或协程worker
    if (workThreadNum > 0) {
        _worker = new MultiThreadWorker(this, workThreadNum);
    } else {
        _worker = new CoroutineWorker(this);
    }
    
    _pipelineFactory = new HttpPipelineFactory(_worker);
}

HttpServer::~HttpServer() {}

HttpServer* HttpServer::create(IO *io, uint16_t workThreadNum, const std::string& ip, uint16_t port) {
    assert(io);
    HttpServer *server = new HttpServer(io, workThreadNum, ip, port);

    // TODO: 注册默认filter
    
    server->start();
    return server;
}

corpc::Connection *HttpServer::buildConnection(int fd) {
    return new HttpConnection(fd, this);
}

void HttpServer::onClose(std::shared_ptr<corpc::Connection>& connection) {
    LOG("HttpServer::onClose -- connection fd:%d is closed\n", connection->getfd());
}

void HttpServer::registerFilter(HttpFilter filter) {
    _filterVec.push_back(filter);
}

void HttpServer::registerHandler(HttpMethod method, const std::string& uri, HttpHandler handler) {
    switch (method) {
        case GET: {
            _getterMap.insert(std::make_pair(uri, handler));
            break;
        }
        case POST: {
            _posterMap.insert(std::make_pair(uri, handler));
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
        handlerIt = _getterMap.find(request->getURI());

        if (handlerIt == _getterMap.end()) {
            response->setResult(HttpStatusCode::NotFound);
            return send(connection, request, response);
        }
    } else if (strcmp(request->getMethod().c_str(), "POST") == 0) {
        handlerIt = _posterMap.find(request->getURI());

        if (handlerIt == _posterMap.end()) {
            response->setResult(HttpStatusCode::NotFound);
            return send(connection, request, response);
        }
    } else {
        response->setResult(HttpStatusCode::NotFound);
        return send(connection, request, response);
    }

    // 先执行filter, 再执行handler
    if (!_filterVec.empty()) {
        for (auto &filter : _filterVec) {
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

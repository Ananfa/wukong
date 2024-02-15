#ifndef wukong_http_server_h
#define wukong_http_server_h

#include "corpc_io.h"
#include "http_parser.h"
#include "http_message.h"

#include <stdio.h>
#include <map>

#define HTTP_PIPELINE_CLEAN_BUF_THRESHOLD 0x100000

using namespace corpc;

namespace wukong {
    enum HttpMethod {
        GET,
        POST
    };
    
    class HttpPipeline: public Pipeline {
    public:
        HttpPipeline(std::shared_ptr<corpc::Connection> &connection, Worker *worker);
        virtual ~HttpPipeline() {}
        
        virtual bool upflow(uint8_t *buf, int size);
        virtual bool downflow(uint8_t *buf, int space, int &size);
        
    private:
        std::shared_ptr<HttpParser> parser_;
        
        std::string buf_; // 缓存数据
        uint32_t nparsed_; // _buf中已处理的数量（注意：当_nparsed大于HTTP_PIPELINE_CLEAN_BUF_THRESHOLD时会触发将_buf中已处理数据清理逻辑）
    };
    
    class HttpPipelineFactory: public PipelineFactory {
    public:
        HttpPipelineFactory(Worker *worker): PipelineFactory(worker) {}
        ~HttpPipelineFactory() {}
        
        virtual std::shared_ptr<Pipeline> buildPipeline(std::shared_ptr<corpc::Connection> &connection);
    };
    
    class HttpServer;
    
    class HttpConnection: public corpc::Connection {
    public:
        HttpConnection(int fd, HttpServer* server);
        virtual ~HttpConnection();
        
        virtual void onClose();
        
        HttpServer *getServer() { return server_; }
    private:
        HttpServer *server_;
        
        std::string responseBuf_;       // 当前正在发送的response序列化产生的数据
        uint32_t responseBufSentNum_;   // 已发送的数据量
    public:
        friend class HttpPipeline;
    };
    
    struct HttpWorkerTask {
        std::shared_ptr<HttpConnection> connection;
        std::shared_ptr<RequestMessage> request;
        std::shared_ptr<ResponseMessage> response;
    };
    
    typedef std::function<bool (std::shared_ptr<RequestMessage>&, std::shared_ptr<ResponseMessage>&)> HttpFilter;
    typedef std::function<void (std::shared_ptr<RequestMessage>&, std::shared_ptr<ResponseMessage>&)> HttpHandler;
    
    class HttpServer: public corpc::Server {
        
        class MultiThreadWorker: public corpc::MultiThreadWorker {
        public:
            MultiThreadWorker(HttpServer *server, uint16_t threadNum): corpc::MultiThreadWorker(threadNum), server_(server) {}
            virtual ~MultiThreadWorker() {}
            
        protected:
            static void *taskCallRoutine( void * arg );
            
            virtual void handleMessage(void *msg); // 注意：处理完消息需要自己删除msg
            
        private:
            HttpServer *server_;
        };
        
        class CoroutineWorker: public corpc::CoroutineWorker {
        public:
            CoroutineWorker(HttpServer *server): server_(server) {}
            virtual ~CoroutineWorker() {}
            
        protected:
            static void *taskCallRoutine( void * arg );
            
            virtual void handleMessage(void *msg); // 注意：处理完消息需要自己删除msg
            
        private:
            HttpServer *server_;
        };
        
        
    private:
        HttpServer(IO *io, uint16_t workThreadNum, const std::string& ip, uint16_t port);
        virtual ~HttpServer();  // 不允许在栈上创建server
        
        void send(std::shared_ptr<HttpConnection>& connection, std::shared_ptr<RequestMessage>& request, std::shared_ptr<ResponseMessage>& response);
        void handle(std::shared_ptr<HttpConnection>& connection, std::shared_ptr<RequestMessage>& request, std::shared_ptr<ResponseMessage>& response);
        
    public:
        static HttpServer* create(IO *io, uint16_t workThreadNum, const std::string& ip, uint16_t port);
        
        // override
        virtual corpc::Connection * buildConnection(int fd);
        
        // override
        virtual void onConnect(std::shared_ptr<corpc::Connection>& connection) {}
        
        // override
        virtual void onClose(std::shared_ptr<corpc::Connection>& connection);
        
        // 注册过滤器
        void registerFilter(HttpFilter filter);
        
        // 注册路由处理
        // TODO: 分开Get和Post实现
        void registerHandler(HttpMethod method, const std::string& uri, HttpHandler handler);
        
    private:
        std::vector<HttpFilter> filterVec_;
        std::map<std::string, HttpHandler> getterMap_;
        std::map<std::string, HttpHandler> posterMap_;
        
    public:
        friend class HttpConnection;
    };
}

#endif /* wukong_http_server_h */

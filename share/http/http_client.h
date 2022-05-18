#ifndef wukong_http_client_h
#define wukong_http_client_h

#include <functional>
#include <curl/curl.h>
#include <vector>
#include <string>

namespace wukong {

    class HttpResponse {
    public:
        HttpResponse(int64_t &code, std::string &body): m_code(code), m_body(body) {}
        ~HttpResponse() {}
        
        const int64_t& code() const { return m_code; }
        const std::string& body() const { return m_body; }
        
    private:
        int64_t m_code;
        std::string m_body;
        // TODO: get http response headers
    };

    class HttpRequest {
    public:
        HttpRequest();
        ~HttpRequest();
        
        void setUrl(const std::string &url);
        void setMethod(const std::string &method);
        void setUserAgent(const std::string &userAgent);
        void setTimeout(int32_t timeout);
        
        void addQueryParam(const std::string &name, const std::string &value);
        void setQueryHeader(const std::string &name, const std::string &value);
        void setRequestBody(const std::string &body);
        void setDnsCache(CURLSH *share);
        
        /**
         * @brief HTTP GET请求
         * param done 回调函数
         */
        void doGet(std::function<void(const HttpResponse&)> done);
        /**
         * @brief HTTP POST请求
         * param done 回调函数
         */
        void doPost(std::function<void(const HttpResponse&)> done);
        
    private:
        void prepare();
        void finish();
        void cleanupBefore();
        void cleanupAfter();
        void urlEncode(const std::string &input, std::string &output);
        void paramsJoin(std::string &output);
        
        static size_t writeFunction(char *data, size_t size, size_t nmemb, void *buffer_in);
        static int32_t setSocketOption(void *clientp, curl_socket_t curlfd, curlsocktype purpose);
        
    private:
        struct QueryParam {
            std::string name;
            std::string value;
        };
        
        CURL *m_curl;
        
        // Request stuff
        std::string m_url;
        int32_t m_timeout;
        std::string m_method;
        std::string m_userAgent;
        std::string m_requestBody;
        struct curl_slist *m_headers;
        std::vector<QueryParam> m_queryParams;
        std::string m_param;
        
        // Response stuff
        CURLcode m_result;
        /* Http Response Code
         1xx    Transient code, a new one follows
         2xx    Things are OK
         3xx    The content is somewhere else
         4xx    Failed because of a client problem
         5xx    Failed because of a server problem */
        int64_t m_code;
        std::string m_responseBody;
        char m_errbuf[CURL_ERROR_SIZE];
        
    };

    class HttpClient {
    public:
        static HttpClient& Instance() {
            static HttpClient instance;
            return instance;
        }
        
        void init() {}
        void doGet(HttpRequest *request, std::function<void(const HttpResponse&)> done);
        void doPost(HttpRequest *request,std::function<void(const HttpResponse&)> done);
        
    private:
        CURLSH *m_curlCache;
        
    private:
        HttpClient();                                     // ctor hidden
        ~HttpClient();                                    // destruct hidden
        HttpClient(HttpClient const&) = delete;           // copy ctor delete
        HttpClient(HttpClient &&) = delete;               // move ctor delete
        HttpClient& operator=(HttpClient const&) = delete;// assign op. delete
        HttpClient& operator=(HttpClient &&) = delete;    // move assign op. delete
    };

}

#define g_HttpClient HttpClient::Instance()

#endif /* wukong_http_client_h */

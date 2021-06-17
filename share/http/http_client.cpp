#include "http_client.h"
#include "corpc_utils.h"

#include <cstring>
#include <assert.h>

using namespace wukong;

HttpRequest::HttpRequest()
    : m_curl(nullptr)
    , m_headers(nullptr)
    , m_code(-1)
    , m_timeout(0)
    , m_url("")
    , m_method("")
    , m_userAgent("")
    , m_responseBody("")
    , m_requestBody("")
    , m_param("") {
    m_curl = curl_easy_init();
    
    curl_easy_setopt(m_curl, CURLOPT_COOKIELIST, "");
    setUserAgent("Mozilla/5.0");
    
    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &m_responseBody);
    curl_easy_setopt(m_curl, CURLOPT_ERRORBUFFER, m_errbuf);
    
    curl_easy_setopt(m_curl, CURLOPT_SOCKOPTFUNCTION, &setSocketOption);
    curl_easy_setopt(m_curl, CURLOPT_SOCKOPTDATA, this);
    
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYHOST, 0L);
    
    curl_easy_setopt(m_curl, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(m_curl, CURLOPT_BUFFERSIZE, 32768L);
    curl_easy_setopt(m_curl, CURLOPT_VERBOSE, 0L);
}

HttpRequest::~HttpRequest() {
    curl_easy_cleanup(m_curl);
    m_curl = nullptr;
    
    if (m_headers != nullptr) {
        curl_slist_free_all( m_headers );
        m_headers = nullptr;
    }
}

void HttpRequest::setUrl(const std::string &url) {
    m_url = url;
}

void HttpRequest::setMethod(const std::string &method) {
    m_method = method;
}

void HttpRequest::setUserAgent(const std::string &userAgent) {
    m_userAgent = userAgent;
}

void HttpRequest::setTimeout(int32_t timeout) {
    if (timeout > 0) {
        m_timeout = timeout;
        curl_easy_setopt(m_curl, CURLOPT_TIMEOUT_MS, m_timeout);
        curl_easy_setopt(m_curl, CURLOPT_CONNECTTIMEOUT_MS, m_timeout);
    }
}

void HttpRequest::addQueryParam(const std::string &name, const std::string &value) {
    QueryParam param;
    param.name = name;
    param.value = value;
    m_queryParams.push_back(param);
}

void HttpRequest::setQueryHeader(const std::string &name, const std::string &value) {
    std::string header;
    
    header += name;
    header += ":";
    header += value;
    
    m_headers = curl_slist_append(m_headers, header.c_str());
}

void HttpRequest::setRequestBody(const std::string &body) {
    m_requestBody = body;
}

void HttpRequest::setDnsCache(CURLSH *share) {
    curl_easy_setopt(m_curl, CURLOPT_SHARE, share);
    curl_easy_setopt(m_curl, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5);
}

void HttpRequest::doGet(std::function<void(const HttpResponse&)> done) {
    setMethod("GET");
    prepare();
    
    m_result = curl_easy_perform(m_curl);
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_code);
    
    if (m_result == CURLE_OK) {
        HttpResponse response(m_code, m_responseBody);
        done(response);
    } else {
        ERROR_LOG("doGet, msg: %s\n", std::strlen(m_errbuf) ? m_errbuf : curl_easy_strerror(m_result));
    }
    
    finish();
}

void HttpRequest::doPost(std::function<void(const HttpResponse&)> done) {
    setMethod("POST");
    prepare();
    
    m_result = curl_easy_perform(m_curl);
    curl_easy_getinfo(m_curl, CURLINFO_RESPONSE_CODE, &m_code);
    
    if (m_result == CURLE_OK) {
        HttpResponse response(m_code, m_responseBody);
        done(response);
    } else {
        /* if the request did not complete correctly, show the error
         information. if no detailed error information was written to errbuf
         show the more generic information from curl_easy_strerror instead. */
        ERROR_LOG("doPost, msg: %s\n", std::strlen(m_errbuf) ? m_errbuf : curl_easy_strerror(m_result));
    }
    
    finish();
}

void HttpRequest::prepare() {
    cleanupBefore();
    std::string fullUrl = m_url;
    m_param = "";
    paramsJoin(m_param);
    
    curl_easy_setopt(m_curl, CURLOPT_USERAGENT, m_userAgent.c_str());
    if (m_headers != nullptr) {
        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, m_headers);
    }
    
    curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 0L);
    if (m_method == "POST") {
        curl_easy_setopt(m_curl, CURLOPT_POST, 1L);
        
        if (m_requestBody.empty()) {
            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, m_param.c_str());
            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, m_param.size());
        } else {
            if (!m_param.empty()) {
                fullUrl += "?";
                fullUrl += m_param;
            }
            
            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, m_requestBody.c_str());
            curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, m_requestBody.size());
        }
        
    } else if (m_method == "GET") {
        curl_easy_setopt(m_curl, CURLOPT_HTTPGET, 1L);
        
        if (!m_param.empty()) {
            fullUrl += "?";
            fullUrl += m_param;
        }
    } else if (m_method == "PUT") {
        curl_easy_setopt(m_curl, CURLOPT_UPLOAD, 1L);
        
        if (!m_param.empty()) {
            fullUrl += "?";
            fullUrl += m_param;
        }
        
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, m_requestBody.c_str());
        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDSIZE, m_requestBody.size());
    } else if (!m_method.empty()) {
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, m_method.c_str());
    } else {
        curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, NULL);
    }
    
    curl_easy_setopt(m_curl, CURLOPT_URL, fullUrl.c_str());
}

void HttpRequest::finish() {
    cleanupAfter();
}

void HttpRequest::cleanupBefore() {
    m_code = -1;
    m_responseBody.clear();
    /* set the error buffer as empty before performing a request */
    m_errbuf[0] = 0;
}

void HttpRequest::cleanupAfter() {
    m_url = "";
    m_method = "";
    m_queryParams.clear();
    if (m_headers) {
        curl_slist_free_all(m_headers);
        m_headers = 0;
    }
}

void HttpRequest::urlEncode(const std::string &input, std::string &output) {
    char *encoded = curl_easy_escape(m_curl, input.c_str() , (int32_t)input.length());
    output = encoded;
    output += "";
    curl_free(encoded);
}

void HttpRequest::paramsJoin(std::string &output) {
    std::vector<QueryParam>::iterator it, end = m_queryParams.end();
    
    for(it = m_queryParams.begin(); it != end; ++it) {
        if (it != m_queryParams.begin()) {
            output += "&";
        }
        
        std::string name, value;
        urlEncode(it->name, name);
        urlEncode(it->value, value);
        output += name + "=" + value;
    }
}

size_t HttpRequest::writeFunction(char *data, size_t size, size_t nmemb, void *buffer_in) {
    size_t totalSize = size * nmemb;
    std::string *body = (std::string *)buffer_in;
    
    if (body != NULL) {
        body->append((char *)data, totalSize);
    }
    
    return totalSize;
}

int32_t HttpRequest::setSocketOption(void *clientp, curl_socket_t curlfd, curlsocktype purpose) {
    struct linger ling;
    ling.l_onoff = 1;
    ling.l_linger = 0;
    setsockopt(curlfd, SOL_SOCKET, SO_LINGER, (void *)&ling, sizeof(ling));
    
    return 0;
}

HttpClient::HttpClient()
    : m_curlCache(nullptr) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    // DnsCache
    m_curlCache = curl_share_init();
    assert(m_curlCache != nullptr && "HttpClient init DnsCache failed");
    curl_share_setopt(m_curlCache, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
    if (m_curlCache != nullptr) {
        curl_share_cleanup(m_curlCache);
        m_curlCache = nullptr;
    }
}

void HttpClient::doGet(HttpRequest *request, std::function<void(const HttpResponse&)> done) {
    request->setDnsCache(m_curlCache);
    request->doGet(done);
}

void HttpClient::doPost(HttpRequest *request, std::function<void(const HttpResponse&)> done) {
    request->setDnsCache(m_curlCache);
    request->doPost(done);
}

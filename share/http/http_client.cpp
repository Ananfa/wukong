#include "http_client.h"
#include "corpc_utils.h"

#include <cstring>
#include <assert.h>

using namespace wukong;

HttpRequest::HttpRequest()
    : curl_(nullptr)
    , headers_(nullptr)
    , code_(-1)
    , timeout_(0)
    , url_("")
    , method_("")
    , userAgent_("")
    , responseBody_("")
    , requestBody_("")
    , param_("") {
    curl_ = curl_easy_init();
    
    curl_easy_setopt(curl_, CURLOPT_COOKIELIST, "");
    setUserAgent("Mozilla/5.0");
    
    curl_easy_setopt(curl_, CURLOPT_WRITEFUNCTION, writeFunction);
    curl_easy_setopt(curl_, CURLOPT_WRITEDATA, &responseBody_);
    curl_easy_setopt(curl_, CURLOPT_ERRORBUFFER, errbuf_);
    
    curl_easy_setopt(curl_, CURLOPT_SOCKOPTFUNCTION, &setSocketOption);
    curl_easy_setopt(curl_, CURLOPT_SOCKOPTDATA, this);
    
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl_, CURLOPT_SSL_VERIFYHOST, 0L);
    
    curl_easy_setopt(curl_, CURLOPT_AUTOREFERER, 1L);
    curl_easy_setopt(curl_, CURLOPT_BUFFERSIZE, 32768L);
    curl_easy_setopt(curl_, CURLOPT_VERBOSE, 0L);
}

HttpRequest::~HttpRequest() {
    curl_easy_cleanup(curl_);
    curl_ = nullptr;
    
    if (headers_ != nullptr) {
        curl_slist_free_all( headers_ );
        headers_ = nullptr;
    }
}

void HttpRequest::setUrl(const std::string &url) {
    url_ = url;
}

void HttpRequest::setMethod(const std::string &method) {
    method_ = method;
}

void HttpRequest::setUserAgent(const std::string &userAgent) {
    userAgent_ = userAgent;
}

void HttpRequest::setTimeout(int32_t timeout) {
    if (timeout > 0) {
        timeout_ = timeout;
        curl_easy_setopt(curl_, CURLOPT_TIMEOUT_MS, timeout_);
        curl_easy_setopt(curl_, CURLOPT_CONNECTTIMEOUT_MS, timeout_);
    }
}

void HttpRequest::addQueryParam(const std::string &name, const std::string &value) {
    QueryParam param;
    param.name = name;
    param.value = value;
    queryParams_.push_back(param);
}

void HttpRequest::setQueryHeader(const std::string &name, const std::string &value) {
    std::string header;
    
    header += name;
    header += ":";
    header += value;
    
    headers_ = curl_slist_append(headers_, header.c_str());
}

void HttpRequest::setRequestBody(const std::string &body) {
    requestBody_ = body;
}

void HttpRequest::setDnsCache(CURLSH *share) {
    curl_easy_setopt(curl_, CURLOPT_SHARE, share);
    curl_easy_setopt(curl_, CURLOPT_DNS_CACHE_TIMEOUT, 60 * 5);
}

void HttpRequest::doGet(std::function<void(const HttpResponse&)> done) {
    setMethod("GET");
    prepare();
    
    result_ = curl_easy_perform(curl_);
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &code_);
    
    if (result_ == CURLE_OK) {
        HttpResponse response(code_, responseBody_);
        done(response);
    } else {
        ERROR_LOG("doGet, msg: %s\n", std::strlen(errbuf_) ? errbuf_ : curl_easy_strerror(result_));
    }
    
    finish();
}

void HttpRequest::doPost(std::function<void(const HttpResponse&)> done) {
    setMethod("POST");
    prepare();
    
    result_ = curl_easy_perform(curl_);
    curl_easy_getinfo(curl_, CURLINFO_RESPONSE_CODE, &code_);
    
    if (result_ == CURLE_OK) {
        HttpResponse response(code_, responseBody_);
        done(response);
    } else {
        /* if the request did not complete correctly, show the error
         information. if no detailed error information was written to errbuf
         show the more generic information from curl_easy_strerror instead. */
        ERROR_LOG("doPost, msg: %s\n", std::strlen(errbuf_) ? errbuf_ : curl_easy_strerror(result_));
    }
    
    finish();
}

void HttpRequest::prepare() {
    cleanupBefore();
    std::string fullUrl = url_;
    param_ = "";
    paramsJoin(param_);
    
    curl_easy_setopt(curl_, CURLOPT_USERAGENT, userAgent_.c_str());
    if (headers_ != nullptr) {
        curl_easy_setopt(curl_, CURLOPT_HTTPHEADER, headers_);
    }
    
    curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(curl_, CURLOPT_UPLOAD, 0L);
    if (method_ == "POST") {
        curl_easy_setopt(curl_, CURLOPT_POST, 1L);
        
        if (requestBody_.empty()) {
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, param_.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, param_.size());
        } else {
            if (!param_.empty()) {
                fullUrl += "?";
                fullUrl += param_;
            }
            
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, requestBody_.c_str());
            curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, requestBody_.size());
        }
        
    } else if (method_ == "GET") {
        curl_easy_setopt(curl_, CURLOPT_HTTPGET, 1L);
        
        if (!param_.empty()) {
            fullUrl += "?";
            fullUrl += param_;
        }
    } else if (method_ == "PUT") {
        curl_easy_setopt(curl_, CURLOPT_UPLOAD, 1L);
        
        if (!param_.empty()) {
            fullUrl += "?";
            fullUrl += param_;
        }
        
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDS, requestBody_.c_str());
        curl_easy_setopt(curl_, CURLOPT_POSTFIELDSIZE, requestBody_.size());
    } else if (!method_.empty()) {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, method_.c_str());
    } else {
        curl_easy_setopt(curl_, CURLOPT_CUSTOMREQUEST, NULL);
    }
    
    curl_easy_setopt(curl_, CURLOPT_URL, fullUrl.c_str());
}

void HttpRequest::finish() {
    cleanupAfter();
}

void HttpRequest::cleanupBefore() {
    code_ = -1;
    responseBody_.clear();
    /* set the error buffer as empty before performing a request */
    errbuf_[0] = 0;
}

void HttpRequest::cleanupAfter() {
    url_ = "";
    method_ = "";
    queryParams_.clear();
    if (headers_) {
        curl_slist_free_all(headers_);
        headers_ = 0;
    }
}

void HttpRequest::urlEncode(const std::string &input, std::string &output) {
    char *encoded = curl_easy_escape(curl_, input.c_str() , (int32_t)input.length());
    output = encoded;
    output += "";
    curl_free(encoded);
}

void HttpRequest::paramsJoin(std::string &output) {
    std::vector<QueryParam>::iterator it, end = queryParams_.end();
    
    for(it = queryParams_.begin(); it != end; ++it) {
        if (it != queryParams_.begin()) {
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
    : curlCache_(nullptr) {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    // DnsCache
    curlCache_ = curl_share_init();
    assert(curlCache_ != nullptr && "HttpClient init DnsCache failed");
    curl_share_setopt(curlCache_, CURLSHOPT_SHARE, CURL_LOCK_DATA_DNS);
}

HttpClient::~HttpClient() {
    curl_global_cleanup();
    if (curlCache_ != nullptr) {
        curl_share_cleanup(curlCache_);
        curlCache_ = nullptr;
    }
}

void HttpClient::doGet(HttpRequest *request, std::function<void(const HttpResponse&)> done) {
    request->setDnsCache(curlCache_);
    request->doGet(done);
}

void HttpClient::doPost(HttpRequest *request, std::function<void(const HttpResponse&)> done) {
    request->setDnsCache(curlCache_);
    request->doPost(done);
}

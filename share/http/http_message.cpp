#include <cstdio>
#include <cstring>
#include <sys/time.h>

#include "http_message.h"

#include "corpc_utils.h"

using namespace wukong;

const char * HttpMessage::HEADER_DATE               = "Date";
const char * HttpMessage::HEADER_SERVER             = "Server";
const char * HttpMessage::HEADER_KEEPALIVE          = "Keep-Alive";
const char * HttpMessage::HEADER_CONNECTION         = "Connection";
const char * HttpMessage::HEADER_CONTENT_TYPE       = "Content-Type";
const char * HttpMessage::HEADER_CONTENT_LENGTH     = "Content-Length";
const char * HttpMessage::HEADER_PROXY_CONNECTION   = "Proxy-Connection";
const char * HttpMessage::HEADER_TRANSFER_ENCODING  = "Transfer-Encoding";

HttpMessage::HttpMessage(int8_t type)
    : type_(type)
    , version_("HTTP/1.1")
    , capacity_(0) {}

HttpMessage::~HttpMessage() {}

void HttpMessage::printAll() const {
    LOG("HttpMessage {\n");
    LOG("\tVersion: %s\n", this->getVersion().c_str());

    for (Headers::const_iterator it = headers_.begin(); it != headers_.end(); ++it) {
        LOG("\t%s: %s\n", it->first.c_str(), it->second.c_str());
    }

    LOG("\tContent: %s\n", content_.c_str());
    LOG("}\n");
}

bool HttpMessage::isHaveHeader(const char * key) const {
    return headers_.find(key) != headers_.end();
}

void HttpMessage::addHeader(const char * key, const char *value) {
    headers_.insert(std::make_pair(key, value));
}

const std::string& HttpMessage::operator[] (const std::string &key) {
    return headers_[key];
}

bool HttpMessage::isKeepalive() {
    return (this->isHaveHeader(HEADER_CONNECTION) && strcasecmp(headers_[HEADER_CONNECTION].c_str(), "Keep-Alive") == 0) || (this->isHaveHeader(HEADER_PROXY_CONNECTION) && strcasecmp(headers_[HEADER_PROXY_CONNECTION].c_str(), "Keep-Alive") == 0);
}

////////////////////////////////////////////////////////////////////////////////

RequestMessage::RequestMessage() : HttpMessage(HttpMessage::eRequest) {}

RequestMessage::~RequestMessage() {}

void RequestMessage::printAll() const {
    HttpMessage::printAll();

    LOG("RequestMessage {\n");
    LOG("\tMethod: %s\n", method_.c_str());
    LOG("\tURL: %s, URI: %s\n", url_.c_str(), uri_.c_str());

    for (Params::const_iterator it = params_.begin(); it != params_.end(); ++it) {
        LOG("\t%s: %s\n", it->first.c_str(), it->second.c_str());
    }
    LOG("}\n");
}

void RequestMessage::addParam(const char *key, const char *value) {
    params_.insert(std::make_pair(key, value));
}

const std::string& RequestMessage::operator[] (const std::string &key) {
    return params_[key];
}

////////////////////////////////////////////////////////////////////////////////
ResponseMessage::ResponseMessage()
    : statusCode_(0), HttpMessage(HttpMessage::eResponse) {}

ResponseMessage::~ResponseMessage() {}

void ResponseMessage::setResult(int32_t code) {
    this->setStatusCode(code);
    this->setReasonPhrase(ResponseMessage::reasonPhrase(code).c_str());
}

/*! Returns the standard HTTP reason phrase for a HTTP status code.
 * \param code An HTTP status code.
 * \return The standard HTTP reason phrase for the given \p code or an empty \c std::string()
 * if no standard phrase for the given \p code is known.
 */
std::string ResponseMessage::reasonPhrase(int32_t code) {
    switch (code) {
            
        //####### 1xx - Informational #######
        case 100: return "Continue";
        case 101: return "Switching Protocols";
        case 102: return "Processing";
            
        //####### 2xx - Successful #######
        case 200: return "OK";
        case 201: return "Created";
        case 202: return "Accepted";
        case 203: return "Non-Authoritative Information";
        case 204: return "No Content";
        case 205: return "Reset Content";
        case 206: return "Partial Content";
        case 207: return "Multi-Status";
        case 226: return "IM Used";
            
        //####### 3xx - Redirection #######
        case 300: return "Multiple Choices";
        case 301: return "Moved Permanently";
        case 302: return "Found";
        case 303: return "See Other";
        case 304: return "Not Modified";
        case 305: return "Use Proxy";
        case 307: return "Temporary Redirect";
        case 308: return "Permanent Redirect";
            
        //####### 4xx - Client Error #######
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 402: return "Payment Required";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 406: return "Not Acceptable";
        case 407: return "Proxy Authentication Required";
        case 408: return "Request Timeout";
        case 409: return "Conflict";
        case 410: return "Gone";
        case 411: return "Length Required";
        case 412: return "Precondition Failed";
        case 413: return "Payload Too Large";
        case 414: return "URI Too Long";
        case 415: return "Unsupported Media Type";
        case 416: return "Range Not Satisfiable";
        case 417: return "Expectation Failed";
        case 418: return "I'm a teapot";
        case 422: return "Unprocessable Entity";
        case 423: return "Locked";
        case 424: return "Failed Dependency";
        case 426: return "Upgrade Required";
        case 428: return "Precondition Required";
        case 429: return "Too Many Requests";
        case 431: return "Request Header Fields Too Large";
        case 451: return "Unavailable For Legal Reasons";
            
        //####### 5xx - Server Error #######
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        case 504: return "Gateway Time-out";
        case 505: return "HTTP Version Not Supported";
        case 506: return "Variant Also Negotiates";
        case 507: return "Insufficient Storage";
        case 511: return "Network Authentication Required";
            
        default: return std::string();
    }
}

void ResponseMessage::addDefaultHeaders() {
    char buf[1024];

    // HTTPDate
    if (!this->isHaveHeader(HEADER_DATE)) {
        struct tm t;
        time_t now = time(nullptr);
        localtime_r(&now, &t);
        
        strftime(buf, 1023, "%a, %d %b %Y %H:%M:%S %Z", &t);
        this->addHeader(HEADER_DATE, buf);
    }

    // ContentType
    if (!content_.empty() && !this->isHaveHeader(HEADER_CONTENT_TYPE)) {
        this->addHeader(HEADER_CONTENT_TYPE, "text/html; charset=UTF-8");
    }
}

void ResponseMessage::serialize(std::string &buffer) {
    char buf[1024];

    // StartLine
    std::snprintf(buf, 1023, "%s %i %s\r\n", this->getVersion().c_str(), this->getStatusCode(), this->getReasonPhrase().c_str());
    buffer += buf;

    // HEADS
    // 添加默认头参数
    this->addDefaultHeaders();
    // 序列化
    for (Headers::const_iterator it = headers_.begin(); it != headers_.end(); ++it) {
        std::snprintf(buf, 1023, "%s: %s\r\n", it->first.c_str(), it->second.c_str());
        buffer += buf;
    }
    // 未添加ContentLength的HTTP头
    if ( /*!content_.empty() && */!isHaveHeader(HEADER_CONTENT_LENGTH)) {
        std::snprintf(buf, 1023, "%s: %lu\r\n", HEADER_CONTENT_LENGTH, content_.size());
        buffer += buf;
    }
    // HEAD结束符
    buffer += "\r\n";

    // CONTENT
    buffer += content_;
}

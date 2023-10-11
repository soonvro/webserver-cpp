#include "HttpResponse.hpp"

// Getters
const unsigned short& HttpResponse::getHttpMajor() const {
    return _http_major;
}

const unsigned short& HttpResponse::getHttpMinor() const {
    return _http_minor;
}

const unsigned short& HttpResponse::getStatus() const {
    return _status;
}

const std::string& HttpResponse::getStatusMessage() const {
    return _status_message;
}

const std::map<std::string, std::string>& HttpResponse::getHeader() const {
    return _headers;
}

const unsigned long long& HttpResponse::getContentLength() const {
    return _content_length;
}

const bool& HttpResponse::getIsChunked() const {
    return _is_chunked;
}

const char* HttpResponse::getBody() const {
    return _body;
}

// Setters
void HttpResponse::setHttpMajor(unsigned short http_major) {
    _http_major = http_major;
}

void HttpResponse::setHttpMinor(unsigned short http_minor) {
    _http_minor = http_minor;
}

void HttpResponse::setStatus(unsigned short status) {
    _status = status;
}

void HttpResponse::setStatusMessage(const std::string& status_message) {
    _status_message = status_message;
}

void HttpResponse::setHeaders(const std::map<std::string, std::string>& headers) {
    _headers = headers;
}

void HttpResponse::setContentLength(unsigned long long content_length) {
    _content_length = content_length;
}

void HttpResponse::setIsChunked(bool is_chunked) {
    _is_chunked = is_chunked;
}

void HttpResponse::setBody(const char* body) {
    _body = const_cast<char*>(body);
}
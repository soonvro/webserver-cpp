#include "HttpRequest.hpp"

// Getters
const HPS::Method& HttpRequest::getMethod() const {
    return _method;
}

const std::string& HttpRequest::getHost() const {
    return _host;
}

const std::string& HttpRequest::getLocation() const {
    return _location;
}

const std::vector<std::pair<std::string, std::string> >& HttpRequest::getQueries() const {
    return _queries;
}

const unsigned short& HttpRequest::getHttpMajor() const {
    return _http_major;
}

const unsigned short& HttpRequest::getHttpMinor() const {
    return _http_minor;
}

const std::map<std::string, std::string>& HttpRequest::getHeaders() const {
    return _headers;
}

const unsigned long long& HttpRequest::getContentLength() const {
    return _content_length;
}

const bool& HttpRequest::getIsChunked() const {
    return _is_chunked;
}

const std::vector<char> HttpRequest::getEntity() const {
    return _entity;
}

const bool& HttpRequest::getHeaderArrived() const {
    return _header_arrived;
}

const bool& HttpRequest::getEntityArrived() const {
    return _entity_arrived;
}


// Setters
void HttpRequest::setMethod(HPS::Method method) {
    _method = method;
}

void HttpRequest::setHost(const std::string& host) {
    _host = host;
}

void HttpRequest::setLocation(const std::string& location) {
    _location = location;
}

void HttpRequest::setQueries(const std::vector<std::pair<std::string, std::string> >& queries) {
    _queries = queries;
}

void HttpRequest::setHttpMajor(unsigned short httpMajor) {
    _http_major = httpMajor;
}

void HttpRequest::setHttpMinor(unsigned short httpMinor) {
    _http_minor = httpMinor;
}

void HttpRequest::setHeaders(const std::map<std::string, std::string>& headers) {
    _headers = headers;
}

void HttpRequest::setContentLength(unsigned long long contentLength) {
    _content_length = contentLength;
}

void HttpRequest::setIsChunked(bool isChunked) {
    _is_chunked = isChunked;
}

void HttpRequest::setEntity(const std::vector<char>& entity) {
    _entity = entity;
}

void HttpRequest::setHeaderArrived(bool headerArrived) {
    _header_arrived = headerArrived;
}

void HttpRequest::setEntityArrived(bool entityArrived) {
    _entity_arrived = entityArrived;
}

/*  call back example

bool on_message_begin(HttpDecoder* hd) {
  return 0; 
}
static bool sOnMessageBegin(HttpDecoder* hd){
  HttpRequest* pthis = static_cast<HttpRequest*>(hd->_data);
  hd = 0;
  return pthis->on_message_begin(hd);
}

*/
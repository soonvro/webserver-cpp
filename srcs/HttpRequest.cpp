#include "HttpRequest.hpp"

const enum HttpDecoderMethod&                             HttpRequest::getMethod(void) const return _method;
const std::string&                                        HttpRequest::getHost(void) const  return _host;
const unsigned short&                                     HttpRequest::getPort(void) const  return _port;
const std::string&                                        HttpRequest::getLocation(void) const return _location;
const std::vector<std::pair<std::string, std::string> >&  HttpRequest::getQueries() const return _queries;
const unsigned short&                                     HttpRequest::getHttpMajor(void) const return _http_major;
const unsigned short&                                     HttpRequest::getHttpMinor(void) const return _http_minor;

const std::map<std::string, std::string>&                 HttpRequest::getHeaders(void) const return _headers;
const unsigned long long&                                 HttpRequest::getContentLength(void) const return _content_length;
const bool&                                               HttpRequest::getIsChunked(void) const return _is_chunked;

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
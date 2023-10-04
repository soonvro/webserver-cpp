#ifndef HTTTP_REQUEST_HPP_
#define HTTTP_REQUEST_HPP_

#include "HttpDecoder.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

class HttpRequest{
  private:
    //client request
    char* _buf;

    //start line
    enum HttpDecoderMethod _method;
  
    std::string            _host;
    unsigned short         _port;
    std::string            _location;
    std::vector<std::pair<std::string, std::string>> _queries;
    unsigned short         _http_major;
    unsigned short         _http_minor;

    //headers
    std::map<std::string, std::string> _headers;
    unsigned long long      _content_length;
    bool                    _is_chunked;

    //entity
    std::string _entity;
    
    bool on_message_begin(HttpDecoder* hd) {
      
    }
    
  public:
    static bool sOnMessageBegin(HttpDecoder* hd){
      HttpRequest* pthis = static_cast<HttpRequest*>(hd->_data);
      return pthis->on_message_begin(hd);
    }
    
    const enum HttpDecoderMethod&                           getMethod(void) const;
    const std::string&                                      getHost(void) const;
    const unsigned short&                                   getPort(void) const;
    const std::string&                                      getLocation(void) const;
    const std::vector<std::pair<std::string, std::string>>& getQueries() const;
    const unsigned short&                                   getHttpMajor(void) const;
    const unsigned short&                                   getHttpMinor(void) const;

    const std::map<std::string, std::string>&               getHeaders(void) const;
    const unsigned long long&                               getContentLength(void) const;
    const bool&                                             getIsChunked(void) const;
};
#endif
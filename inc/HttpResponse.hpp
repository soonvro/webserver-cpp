#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_

#include "HttpDecoder.h"
#include <string>
#include <map>

class HttpResponse {
  private:

    // start line
    std::string                         _host;
    unsigned short                      _port;

    unsigned short                      _http_major;
    unsigned short                      _http_minor;

    // headers
    std::map<std::string, std::string>  _headers;
    unsigned long long                  _content_length;
    bool                                _is_chunked;

  public:
    const std::string&                        getHost(void) const;
    const unsigned short&                     getPort(void) const;
    const unsigned short&                     getHttpMajor(void) const;
    const unsigned short&                     getHttpMinor(void) const;

    const std::map<std::string, std::string>& getHeader(void) const;
    const unsigned long long&                 getContentLength(void) const;
    const bool&                               getIsChunked(void) const;
};

#endif
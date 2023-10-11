#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_

#include "HttpDecoder.h"
#include <string>
#include <map>

class HttpResponse {
  private:

    // start line
    unsigned short                            _http_major;
    unsigned short                            _http_minor;

    unsigned short                            _status;
    std::string                               _status_message;

    // headers
    std::map<std::string, std::string>        _headers;
    unsigned long long                        _content_length;
    bool                                      _is_chunked;

  public:
    const unsigned short&                     getHttpMajor(void) const;
    const unsigned short&                     getHttpMinor(void) const;

    const unsigned short&                     getStatus(void) const;
    const std::string&                        getStatusMessage(void) const;

    const std::map<std::string, std::string>& getHeader(void) const;
    const unsigned long long&                 getContentLength(void) const;
    const bool&                               getIsChunked(void) const;

    void                                      setHttpMajor(unsigned short http_major);
    void                                      setHttpMinor(unsigned short http_minor);
    void                                      setStatus(unsigned short status);
    void                                      setStatusMessage(const std::string& status_message);
    void                                      setHeaders(const std::map<std::string, std::string>& headers);
    void                                      setContentLength(unsigned long long content_length);
    void                                      setIsChunked(bool is_chunked);
};

#endif
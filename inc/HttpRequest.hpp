#ifndef HTTTP_REQUEST_HPP_
#define HTTTP_REQUEST_HPP_

#include "HttpDecoder.h"
#include <map>
#include <string>
#include <utility>
#include <vector>

class HttpRequest{
  private:
    //start line
    enum HPS::Method                                          _method;
  
    std::string                                               _host;
    std::string                                               _location;
    std::vector<std::pair<std::string, std::string> >         _queries;
    unsigned short                                            _http_major;
    unsigned short                                            _http_minor;

    //headers
    std::map<std::string, std::string>                        _headers;
    unsigned long long                                        _content_length;
    bool                                                      _is_chunked;

    //entity
    std::vector<char>                                         _entity;

    bool                                                      _header_arrived;
    bool                                                      _entity_arrived;
  public:
    
    const HPS::Method&                                   getMethod(void) const;
    const std::string&                                        getHost(void) const;
    const std::string&                                        getLocation(void) const;
    const std::vector<std::pair<std::string, std::string> >&  getQueries() const;
    const unsigned short&                                     getHttpMajor(void) const;
    const unsigned short&                                     getHttpMinor(void) const;
    const std::map<std::string, std::string>&                 getHeaders(void) const;
    const unsigned long long&                                 getContentLength(void) const;
    const bool&                                               getIsChunked(void) const;
    const std::vector<char>                                   getEntity(void) const;
    const bool&                                               getHeaderArrived(void) const;
    const bool&                                               getEntityArrived(void) const;

    void                                                      setMethod(HPS::Method method);
    void                                                      setHost(const std::string& host);
    void                                                      setLocation(const std::string& location);
    void                                                      setQueries(const std::vector<std::pair<std::string, std::string> >& queries);
    void                                                      setHttpMajor(unsigned short httpMajor);
    void                                                      setHttpMinor(unsigned short httpMinor);
    void                                                      setHeaders(const std::map<std::string, std::string>& headers);
    void                                                      setContentLength(unsigned long long contentLength);
    void                                                      setIsChunked(bool isChunked);
    void                                                      setEntity(const std::vector<char>& entity);
    void                                                      setHeaderArrived(bool headerArrived);
    void                                                      setEntityArrived(bool entityArrived);
};
#endif
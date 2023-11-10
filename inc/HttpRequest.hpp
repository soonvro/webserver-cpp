#ifndef HTTTP_REQUEST_HPP_
#define HTTTP_REQUEST_HPP_

#include "HttpDecoder.hpp"
#include <map>
#include <string>
#include <utility>
#include <vector>

#define REQUEST_BUF_SIZE 134217728

enum HttpRequestHeader {
  kHeaderNo,
  kHeaderHost,
  kHeaderNomal
};

class HttpRequest {
  public:
    HttpRequest();
    ~HttpRequest();

    HttpRequest(const HttpRequest& other);
    HttpRequest& operator=(const HttpRequest& other);

    const HPS::Method&                        getMethod(void) const;
    const std::string&                        getHost(void) const;
    const std::string&                        getLocation(void) const;
    const std::string&                        getQueries() const;
    const unsigned short&                     getHttpMajor(void) const;
    const unsigned short&                     getHttpMinor(void) const;
    const std::map<std::string, std::string>& getHeaders(void) const;
    const unsigned long long&                 getContentLength(void) const;
    const bool&                               getIsChunked(void) const;
    const std::vector<char>&                  getEntity(void) const;
    const bool&                               getHeaderArrived(void) const;
    const bool&                               getEntityArrived(void) const;
    const std::string                         getHeaderValue(std::string h_field) const;

    int                                       settingContent(std::vector<char>::const_iterator start, std::vector<char>::const_iterator end);
    void                                      setQueries(const std::string& queries);
    void                                      setLocation(const std::string& location);

    static bool sParseUrl(
        HttpDecoder* hd, const char *at, unsigned int len) {
      HttpRequest* pthis = static_cast<HttpRequest*>(hd->_data);
      return pthis->parseUrl(hd, at, len);
    }
    static bool sSaveHeaderField(
        HttpDecoder* hd, const char *at, unsigned int len) {
      HttpRequest* pthis = static_cast<HttpRequest*>(hd->_data);
      return pthis->recognizeHeaderField(hd, at, len);
    }
    static bool sParseHeaderValue(
        HttpDecoder* hd, const char *at, unsigned int len) {
      HttpRequest* pthis = static_cast<HttpRequest*>(hd->_data);
      return pthis->parseHeaderValue(hd, at, len);
    }
    static bool sSaveRquestData( HttpDecoder* hd) {
      HttpRequest* pthis = static_cast<HttpRequest*>(hd->_data);
      return pthis->saveRquestData(hd);
    }

    class ChunkedException : public std::exception {
      public: 
        const char* what() const throw() { return "Chunked is wrong."; }
    };

  private:
    static unsigned char                toLower(unsigned char c);

    bool                                isStrCase(const char* lhs_start, unsigned int lhs_len, const char* rhs);
    bool                                isOnlySlash(std::string& s);

    bool                                parseUrl(HttpDecoder* hd, const char *at, unsigned int len);
    bool                                recognizeHeaderField(HttpDecoder* hd, const char *at, unsigned int len);
    bool                                parseHeaderValue(HttpDecoder* hd, const char *at, unsigned int len);
    bool                                saveRquestData(HttpDecoder* hd);

    void                                chunkedLength(std::vector<char>::const_iterator& start, std::vector<char>::const_iterator& end);
    void                                chunkedSetting(std::vector<char>::const_iterator& start, std::vector<char>::const_iterator& end);

    //start line
    enum HPS::Method                    _method;
    unsigned short                      _http_major;
    unsigned short                      _http_minor;

    std::string                         _host;
    std::string                         _location;
    unsigned int                        _port;
    std::string                         _queries;

    //headers
    enum HttpRequestHeader              _h_field;
    std::string                         _last_headers_key;
    std::map<std::string, std::string>  _headers;
    unsigned long long                  _content_length;

    long long                           _chunked_block_size;
    long long                           _chunked_block_length;

    bool                                _is_host_header_comein;
    bool                                _is_chunked;
    bool                                _is_connection_keep_alive;
    bool                                _is_connection_close;
    bool                                _is_content_length;
    bool                                _has_chunked_len;
    bool                                _pre_cr;

    //entity
    std::vector<char>                   _entity;

    bool                                _header_arrived;
    bool                                _entity_arrived;
};

#endif

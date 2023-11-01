#ifndef HTTPRESPONSE_HPP_
#define HTTPRESPONSE_HPP_

#include "HttpDecoder.hpp"
#include "HttpRequest.hpp"
#include "RouteRule.hpp"
#include "CgiHandler.hpp"
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <iostream>

#define BUF_SIZE 4096

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

    // body
    std::vector<char>                         _body;

    bool                                      _is_ready;
    bool                                      _is_cgi;
    CgiHandler                                _cgi_handler;

    void                                      readDir(const std::string& path);

  //  const static std::map<std::string, std::string> contentTypes;

  public:
    HttpResponse();


    const unsigned short&                     getHttpMajor(void) const;
    const unsigned short&                     getHttpMinor(void) const;

    const unsigned short&                     getStatus(void) const;
    const std::string&                        getStatusMessage(void) const;

    const std::map<std::string, std::string>& getHeader(void) const;
    const unsigned long long&                 getContentLength(void) const;
    const bool&                               getIsChunked(void) const;
    const std::vector<char>&                  getBody(void) const;
    const bool&                               getIsReady(void) const;
    const bool&                               getIsCgi(void) const;
    const int&                                getCgiPipeIn(void) const;
    CgiHandler&                               getCgiHandler(void);


    void                                      setHttpMajor(unsigned short http_major);
    void                                      setHttpMinor(unsigned short http_minor);
    void                                      setStatus(unsigned short status);
    void                                      setStatusMessage(const std::string& status_message);
    void                                      setHeaders(const std::map<std::string, std::string>& headers);
    void                                      setContentLength(unsigned long long content_length);
    void                                      setIsChunked(bool is_chunked);
    void                                      setBody(const std::vector<char>& body);
    void                                      setIsReady(bool is_ready);
    void                                      setIsCgi(bool is_cgi);

    void                                      addContentLength(void);
    void                                      publish(const HttpRequest& req, const RouteRule& r);
    void                                      publishError(int status);
    void                                      setHeader(const std::string& key, const std::string& value); 

    void initializeCgiProcess(HttpRequest& req, RouteRule& rule,
                              const std::string& server_name, const int& port, const int& client_fd) throw(std::runtime_error);
    int  cgiExecute(void) throw(std::runtime_error);
    
    void                                      readFile(const std::string& path);

    class FileNotFoundException : public std::exception {
      public: 
        const char* what() const throw() { return "File not found!"; }
    };
};

// const std::map<std::string, std::string> HttpResponse::contentTypes = {
//                                                               {".html", "text/html"},
//                                                               {".css", "text/css"},
//                                                               {".js", "application/javascript"},
//                                                               {".png", "image/png"},
//                                                               {".jpg", "image/jpeg"},
//                                                               {".jpeg", "image/jpeg"},
//                                                               {".gif", "image/gif"},
//                                                               {".json", "application/json"},
//                                                           };

#endif

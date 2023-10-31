#ifndef CGI_HANDLER_HPP_
#define CGI_HANDLER_HPP_

#include <unistd.h>
#include "HttpRequest.hpp"
#include "RouteRule.hpp"

#define DEFAULT_CONTENT_TYPE "text/html"
#define PIPE_READ  0
#define PIPE_WRITE 1

class CgiHandler {
  public:
    CgiHandler();
    CgiHandler(HttpRequest& req, RouteRule& route_rule,
               const std::string& server_name, const int& port) throw(std::runtime_error);
    CgiHandler(const CgiHandler& other);
    CgiHandler& operator=(const CgiHandler& other);

    const int& getReadPipeFromCgi(void) const;

    void setPipe(void) throw(std::runtime_error);
    void setupCgiEnvp(void);

    int execute(void) throw(std::runtime_error);

    const int&          getClientFd(void) const;
    const std::vector<char>& getBuf(void) const;

    void addBuf(const char* buf, size_t size);
    void closeReadPipe(void);

  private:
    int       _pipe_to_cgi_fd[2];
    int       _pipe_from_cgi_fd[2];

    HttpRequest _req;
    RouteRule _route_rule;

    std::string _server_name;
    int         _port;

    std::vector<char> _buf;
    int               _client_fd;

};


#endif

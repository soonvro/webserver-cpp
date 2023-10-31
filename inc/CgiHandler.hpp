#ifndef CGI_HANDLER_HPP_
#define CGI_HANDLER_HPP_

#include "HttpRequest.hpp"
#include "RouteRule.hpp"

class CgiHandler {
  public:
    CgiHandler();
    CgiHandler(HttpRequest& req, RouteRule& route_rule);
    CgiHandler(const CgiHandler& other);
    CgiHandler& operator=(const CgiHandler& other);

    void execute(void);

    const int& getReadPipe(void) const;
    const int&          getClientFd(void) const;
    const std::vector<char>& getBuf(void) const;

    void addBuf(const char* buf, size_t size);
    void closeReadPipe(void);

  private:
    int       _pipe_fd[2];

    HttpRequest _req;
    RouteRule _route_rule;
    std::vector<char> _buf;
    int               _client_fd;
};


#endif

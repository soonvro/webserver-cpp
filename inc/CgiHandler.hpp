#ifndef CGI_HANDLER_HPP_
#define CGI_HANDLER_HPP_

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"
#include "RouteRule.hpp"

class CgiHandler {
  public:
    CgiHandler();
    CgiHandler(HttpRequest& req, RouteRule& route_rule);
    CgiHandler(const CgiHandler& other);
    CgiHandler& operator=(const CgiHandler& other);

    void execute(void);
    
    const int& getReadPipe(void) const;
  private:
    int       _pipe_fd[2];

    HttpRequest _req;
    RouteRule _route_rule;
};


#endif
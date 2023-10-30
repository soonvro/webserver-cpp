#ifndef CGI_HANDLER_HPP_
#define CGI_HANDLER_HPP_

#include "HttpRequest.hpp"
#include "RouteRule.hpp"

#define DEFAULT_CONTENT_TYPE "text/html"

class CgiHandler {
  public:
    CgiHandler();
    CgiHandler(HttpRequest& req, RouteRule& route_rule) throw(std::runtime_error);
    CgiHandler(const CgiHandler& other);
    CgiHandler& operator=(const CgiHandler& other);


    const int& getReadPipeFromCgi(void) const;

    void setPipe(void) throw(std::runtime_error);
    void setupCgiEnvp(void);

    int execute(void) throw(std::runtime_error);
  private:
    int       _pipe_to_cgi_fd[2];
    int       _pipe_from_cgi_fd[2];

    HttpRequest _req;
    RouteRule _route_rule;
};


#endif

#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class Client {
  private:
    std::string               _buf;
    std::vector<HttpRequest>  _reqs;
    std::vector<HttpResponse> _ress;

  public:
    const std::string&                getBuf(void) const;
    const std::vector<HttpRequest>&   getReqs(void) const;
    const std::vector<HttpResponse>&  getRess(void) const;

    void                              clearBuf(void);

    void                              addBuf(std::string& buf);
    void                              addReqs(HttpRequest& req);
    void                              addRess(HttpResponse& res);
};

#endif
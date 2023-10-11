#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

class Client {
  private:
    std::vector<char>         _buf;
    size_t                    _read_idx;
    std::vector<HttpRequest>  _reqs;
    std::vector<HttpResponse> _ress;

    bool                      _has_eof;
  public:
    const std::vector<char>&          getBuf(void) const;
    const size_t&                     getReadIdx(void) const;
    const std::vector<HttpRequest>&   getReqs(void) const;
    const std::vector<HttpResponse>&  getRess(void) const;
    const bool&                       getHasEof(void) const;

    void                              clearBuf(void);

    void                              addBuf(const char* buf, size_t size);
    void                              addReqs(HttpRequest& req);
    void                              addRess(HttpResponse& res);

    void                              setHasEof(bool has_eof);
};

#endif
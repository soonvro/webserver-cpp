#include "Client.hpp"

const std::vector<char>&          Client::getBuf(void) const { return _buf; }
const size_t&                     Client::getReadIdx(void) const { return _read_idx; }
const std::vector<HttpRequest>&   Client::getReqs(void) const { return _reqs; }
const std::vector<HttpResponse>&  Client::getRess(void) const { return _ress; }
const bool&                       Client::getHasEof(void) const { return _has_eof; }

void                              Client::clearBuf(void) {
  _buf.clear();
  _read_idx = 0;
}

void                              Client::addBuf(const char* buf, size_t size) {
  for (size_t i = 0; i < size; ++i) {
    _buf.push_back(buf[i]);
  }
}
void                              Client::addReadIdx(size_t idx) { _read_idx += idx; }
void                              Client::addReqs(HttpRequest& req) { _reqs.push_back(req); }
void                              Client::addRess(HttpResponse& res) { _ress.push_back(res); }
void                              Client::clearRess(void) {_ress.clear(); }
void                              Client::setHasEof(bool has_eof) { _has_eof = has_eof; }

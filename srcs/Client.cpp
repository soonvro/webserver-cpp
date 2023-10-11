#include "Client.hpp"

const std::string&                Client::getBuf(void) const { return _buf; }
const std::vector<HttpRequest>&   Client::getReqs(void) const { return _reqs; }
const std::vector<HttpResponse>&  Client::getRess(void) const { return _ress; }
const bool&                       Client::getHasEof(void) const { return _has_eof; }

void                              Client::clearBuf(void) { _buf.clear(); }

void                              Client::addBuf(std::string& buf) { _buf += buf; }
void                              Client::addReqs(HttpRequest& req) { _reqs.push_back(req); }
void                              Client::addRess(HttpResponse& res) { _ress.push_back(res); }

void                              Client::setHasEof(bool has_eof) { _has_eof = has_eof; }
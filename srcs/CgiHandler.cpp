#include "CgiHandler.hpp"
#include <unistd.h>

CgiHandler::CgiHandler(){}

CgiHandler::CgiHandler(HttpRequest& req, RouteRule& route_rule) : _req(req), _route_rule(route_rule){
  if (pipe(_pipe_fd) == -1)
    throw std::runtime_error("pipe() failed");
}

CgiHandler::CgiHandler(const CgiHandler& other) : _req(other._req), _route_rule(other._route_rule), _buf(other._buf), _client_fd(other._client_fd){
  _pipe_fd[0] = other._pipe_fd[0];
  _pipe_fd[1] = other._pipe_fd[1];
}

CgiHandler& CgiHandler::operator=(const CgiHandler& other){
  if (this == &other)
    return *this;
  _req = other._req;
  _route_rule = other._route_rule;
  _buf = other._buf;
  _client_fd = other._client_fd;
  _pipe_fd[0] = other._pipe_fd[0];
  _pipe_fd[1] = other._pipe_fd[1];
  return *this;
}

const int&                CgiHandler::getReadPipe(void) const{ return _pipe_fd[0]; }
const int&                CgiHandler::getClientFd(void) const{ return _client_fd; }
const std::vector<char>&  CgiHandler::getBuf(void) const{ return _buf; }

void  CgiHandler::addBuf(const char* buf, size_t size){ _buf.insert(_buf.end(), buf, buf + size); }
void  CgiHandler::closeReadPipe(void){ close(_pipe_fd[0]); }

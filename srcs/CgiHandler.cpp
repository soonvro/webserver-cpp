#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sstream>
#include "CgiHandler.hpp"
#include <unistd.h>

CgiHandler::CgiHandler() {}

CgiHandler::CgiHandler(
    HttpRequest& req, RouteRule& route_rule, const std::string& server_name, const int& port) throw(std::runtime_error)
  : _req(req), _route_rule(route_rule), _server_name(server_name), _port(port) {
  this->setPipe();
}

CgiHandler::CgiHandler(const CgiHandler& other)
  : _req(other._req), _route_rule(other._route_rule) {
  _pipe_from_cgi_fd[0] = other._pipe_from_cgi_fd[0];
  _pipe_from_cgi_fd[1] = other._pipe_from_cgi_fd[1];
  _pipe_to_cgi_fd[0] = other._pipe_to_cgi_fd[0];
  _pipe_to_cgi_fd[1] = other._pipe_to_cgi_fd[1];
}

CgiHandler& CgiHandler::operator=(const CgiHandler& other) {
  _req = other._req;
  _route_rule = other._route_rule;
  _pipe_from_cgi_fd[0] = other._pipe_from_cgi_fd[0];
  _pipe_from_cgi_fd[1] = other._pipe_from_cgi_fd[1];
  _pipe_to_cgi_fd[0] = other._pipe_to_cgi_fd[0];
  _pipe_to_cgi_fd[1] = other._pipe_to_cgi_fd[1];
  return *this;
}

const int& CgiHandler::getReadPipeFromCgi(void) const { return _pipe_from_cgi_fd[0]; }

void CgiHandler::setPipe(void) throw(std::runtime_error) {
  if (pipe(_pipe_from_cgi_fd) != 0) throw std::runtime_error("error: can't open pipe");
  fcntl(_pipe_from_cgi_fd[0], F_SETFL, O_NONBLOCK, FD_CLOEXEC);

  if (pipe(_pipe_to_cgi_fd) != 0) throw std::runtime_error("error: can't open pipe");
}

void CgiHandler::setupCgiEnvp(void) {
  setenv("SERVER_SOFTWARE", "webserv/1.0", 1);

  static const char* methods[] = {GET, HEAD, POST, DELETE};
  std::stringstream ss;

  if (_req.getEntityArrived()) {
    ss << _req.getEntity().size();
    setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
  }
  std::string content_type = _req.getHeaderValue("content-type");
  setenv("CONTENT_TYPE", (content_type.empty() ? DEFAULT_CONTENT_TYPE : content_type.c_str()), 1);

  setenv("REQUEST_METHOD", methods[_req.getMethod() - 1], 1);
  setenv("QUERY_STRING", _req.getQueries().c_str(), 1);

  setenv("SCRIPT_NAME", (_route_rule.getRoot() + _req.getLocation()).c_str(), 1);

  setenv("SERVER_NAME", _server_name.c_str(), 1);
  ss << _port;
  setenv("SERVER_PORT", ss.str().c_str(), 1);
  setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
}

int CgiHandler::execute(void) throw(std::runtime_error) {
  pid_t pid = fork();

  if (pid == -1) throw std::runtime_error("error: fork error!");

  if (pid == 0) {  // Child process
      close(_pipe_from_cgi_fd[PIPE_READ]);
      dup2(_pipe_from_cgi_fd[PIPE_WRITE], STDOUT_FILENO);
      close(_pipe_from_cgi_fd[PIPE_WRITE]);
      close(_pipe_to_cgi_fd[PIPE_WRITE]);
      dup2(_pipe_to_cgi_fd[PIPE_READ], STDIN_FILENO);
      close(_pipe_to_cgi_fd[PIPE_READ]);

      this->setupCgiEnvp();

      if (execve(_route_rule.getCgiPath().c_str(), NULL, NULL) == -1) {
        return (EXIT_FAILURE);
      }
      return 0;  //  This part is actually not reached.
  } else {  // Parent process
      close(_pipe_from_cgi_fd[PIPE_WRITE]);
      close(_pipe_to_cgi_fd[PIPE_READ]);

      if (write(_pipe_to_cgi_fd[PIPE_WRITE], &((_req.getEntity())[0]), _req.getEntity().size()) == -1);
      throw std::runtime_error("error: pipe write error!");
      close(_pipe_to_cgi_fd[PIPE_WRITE]);
  }
  return 0;
}

void  CgiHandler::addBuf(const char* buf, size_t size){ _buf.insert(_buf.end(), buf, buf + size); }

void  CgiHandler::closeReadPipe(void){ close(_pipe_fd[0]); }

const int&                CgiHandler::getClientFd(void) const{ return _client_fd; }
const std::vector<char>&  CgiHandler::getBuf(void) const{ return _buf; }

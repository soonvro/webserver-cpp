#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sstream>
#include "CgiHandler.hpp"
#include <unistd.h>


CgiHandler::CgiHandler() {}

CgiHandler::CgiHandler(
    HttpRequest& req, RouteRule& route_rule, const std::string& server_name, const int& port, const int& client_fd) throw(std::runtime_error)
  : _req(req), _route_rule(route_rule), _server_name(server_name), _port(port), _client_fd(client_fd) {
  this->setPipe();
}

CgiHandler::CgiHandler(const CgiHandler& other)
  : _req(other._req), _route_rule(other._route_rule) {
  _pipe_from_cgi_fd[PIPE_READ] = other._pipe_from_cgi_fd[PIPE_READ];
  _pipe_from_cgi_fd[PIPE_WRITE] = other._pipe_from_cgi_fd[PIPE_WRITE];
  _pipe_to_cgi_fd[PIPE_READ] = other._pipe_to_cgi_fd[PIPE_READ];
  _pipe_to_cgi_fd[PIPE_WRITE] = other._pipe_to_cgi_fd[PIPE_WRITE];
  _server_name = other._server_name;
  _port = other._port;
  _buf = other._buf;
  _client_fd = other._client_fd;
}

CgiHandler& CgiHandler::operator=(const CgiHandler& other) {
  if (this == & other)
    return *this;
  _req = other._req;
  _route_rule = other._route_rule;
  _pipe_from_cgi_fd[PIPE_READ] = other._pipe_from_cgi_fd[PIPE_READ];
  _pipe_from_cgi_fd[PIPE_WRITE] = other._pipe_from_cgi_fd[PIPE_WRITE];
  _pipe_to_cgi_fd[PIPE_READ] = other._pipe_to_cgi_fd[PIPE_READ];
  _pipe_to_cgi_fd[PIPE_WRITE] = other._pipe_to_cgi_fd[PIPE_WRITE];
  _server_name = other._server_name;
  _port = other._port;
  _buf = other._buf;
  _client_fd = other._client_fd;
  return *this;
}

const int& CgiHandler::getReadPipeFromCgi(void) const { return _pipe_from_cgi_fd[PIPE_READ]; }

void CgiHandler::setPipe(void) throw(std::runtime_error) {
  if (pipe(_pipe_from_cgi_fd) != 0) throw std::runtime_error("error: can't open pipe");
  fcntl(_pipe_from_cgi_fd[0], F_SETFL, O_NONBLOCK, FD_CLOEXEC);

  if (pipe(_pipe_to_cgi_fd) != 0) throw std::runtime_error("error: can't open pipe");
  fcntl(_pipe_to_cgi_fd[0], F_SETFL, O_NONBLOCK, FD_CLOEXEC);
}

void CgiHandler::setupCgiEnvp(void) {
  static const char* methods[] = {GET, HEAD, POST, DELETE};
  std::stringstream ss;

  setenv("SERVER_SOFTWARE", "webserv/1.0", 1);
  if (_req.getEntityArrived()) {
    ss << _req.getEntity().size();
    setenv("CONTENT_LENGTH", ss.str().c_str(), 1);
  } else{
    setenv("CONTENT_LENGTH", "0", 1);
  }
  std::string content_type = _req.getHeaderValue("content-type");
  setenv("CONTENT_TYPE", (content_type.empty() ? DEFAULT_CONTENT_TYPE : content_type.c_str()), 1);
  setenv("REDIRECT_STATUS", "CGI", 1);
  setenv("GATEWAY_INTERFACE",  "CGI/1.1", 1);
  setenv("REMOTE_ADDR", "127.0.0.1", 1);
  setenv("REQUEST_METHOD", methods[_req.getMethod() - 1], 1);
  setenv("QUERY_STRING", _req.getQueries().c_str(), 1);
  setenv("SCRIPT_NAME", (_route_rule.getRoot() + _req.getLocation()).c_str(), 1);
  setenv("PATH_INFO", (_route_rule.getRoot() + _req.getLocation()).c_str(), 1);
  
  char current_dir[512];
  getcwd(current_dir, 512);
  std::string slash = "/";
  setenv("PATH_TRANSLATED", (current_dir + slash + _route_rule.getRoot() + _req.getLocation()).c_str(), 1);
  setenv("REQUEST_URI", (_route_rule.getRoot() + _req.getLocation() + "?" + _req.getQueries()).c_str(), 1);
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
      char* argv[2] = {NULL,};
      extern char** environ;
      if (execve(_route_rule.getCgiPath().c_str(), argv, environ) == -1) {
        exit(EXIT_FAILURE);
      }
      return 0;  //  This part is actually not reached.
  } else {  // Parent process
      close(_pipe_from_cgi_fd[PIPE_WRITE]);
      close(_pipe_to_cgi_fd[PIPE_READ]);

      if (write(_pipe_to_cgi_fd[PIPE_WRITE], &((_req.getEntity())[0]), _req.getEntity().size()) == -1)
        throw std::runtime_error("error: pipe write error!");
      close(_pipe_to_cgi_fd[PIPE_WRITE]);
  }
  return pid;
}

const int&                CgiHandler::getClientFd(void) const{ return _client_fd; }
const std::vector<char>&  CgiHandler::getBuf(void) const{ return _buf; }
const RouteRule&          CgiHandler::getRouteRule(void) const { return _route_rule; };

void  CgiHandler::addBuf(const char* buf, size_t size){ _buf.insert(_buf.end(), buf, buf + size); }
void  CgiHandler::closeReadPipe(void){ close(_pipe_from_cgi_fd[PIPE_READ]); }

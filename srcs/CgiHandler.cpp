#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sstream>
#include <string.h>
#include <unistd.h>
#include <iostream>

#include "CgiHandler.hpp"

CgiHandler::CgiHandler(const HttpRequest& req, const RouteRule& route_rule) 
  : _is_write_pipe_to_cgi_closed(true), _is_read_pipe_from_cgi_closed(true), _req(req), _route_rule(route_rule) {
  _buf.reserve(CGI_HANDLER_BUF_SIZE);
}

CgiHandler::CgiHandler(
    const HttpRequest& req, const RouteRule& route_rule, const std::string& server_name, const int& port, const int& client_fd) throw(std::runtime_error)
  : _idx(0), _is_write_pipe_to_cgi_closed(true), _is_read_pipe_from_cgi_closed(true), _req(req), _route_rule(route_rule), _server_name(server_name), _port(port), _client_fd(client_fd) 
 {
  this->setPipe();
  _buf.reserve(CGI_HANDLER_BUF_SIZE);
}

CgiHandler::CgiHandler(const CgiHandler& other)
  : _idx(other._idx), _req(other._req), _route_rule(other._route_rule) {
  _pipe_from_cgi_fd[PIPE_READ] = other._pipe_from_cgi_fd[PIPE_READ];
  _pipe_from_cgi_fd[PIPE_WRITE] = other._pipe_from_cgi_fd[PIPE_WRITE];
  _pipe_to_cgi_fd[PIPE_READ] = other._pipe_to_cgi_fd[PIPE_READ];
  _pipe_to_cgi_fd[PIPE_WRITE] = other._pipe_to_cgi_fd[PIPE_WRITE];
  _server_name = other._server_name;
  _port = other._port;
  _buf.reserve(other._buf.capacity());
  _buf = other._buf;
  _client_fd = other._client_fd;
  _is_read_pipe_from_cgi_closed = other._is_read_pipe_from_cgi_closed;
  _is_write_pipe_to_cgi_closed = other._is_write_pipe_to_cgi_closed;
}

CgiHandler& CgiHandler::operator=(const CgiHandler& other) {
  if (this == & other)
    return *this;
  _idx = other._idx;
  _pipe_from_cgi_fd[PIPE_READ] = other._pipe_from_cgi_fd[PIPE_READ];
  _pipe_from_cgi_fd[PIPE_WRITE] = other._pipe_from_cgi_fd[PIPE_WRITE];
  _pipe_to_cgi_fd[PIPE_READ] = other._pipe_to_cgi_fd[PIPE_READ];
  _pipe_to_cgi_fd[PIPE_WRITE] = other._pipe_to_cgi_fd[PIPE_WRITE];
  _server_name = other._server_name;
  _port = other._port;
  _buf.reserve(other._buf.capacity());
  _buf = other._buf;
  _client_fd = other._client_fd;
  _is_read_pipe_from_cgi_closed = other._is_read_pipe_from_cgi_closed;
  _is_write_pipe_to_cgi_closed = other._is_write_pipe_to_cgi_closed;

  return *this;
}

int                       CgiHandler::getCgiReqEntityIdx(void) { return _idx; }
const HttpRequest&        CgiHandler::getRequest(void) { return _req; }
const int&                CgiHandler::getReadPipeFromCgi(void) const { return _pipe_from_cgi_fd[PIPE_READ]; }
const bool&               CgiHandler::getIsReadPipeFromCgiClosed(void) const { return _is_read_pipe_from_cgi_closed; }

const int&                CgiHandler::getWritePipetoCgi(void) const { return _pipe_to_cgi_fd[PIPE_WRITE]; }
const bool&               CgiHandler::getIsWritePipeToCgiClosed(void) const { return _is_write_pipe_to_cgi_closed; }
const int&                CgiHandler::getPid(void) const { return _pid; }

void CgiHandler::setCgiReqEntityIdx(int idx) { _idx = idx; }
void CgiHandler::setPipe(void) throw(std::runtime_error) {
  if (pipe(_pipe_from_cgi_fd) != 0) throw std::runtime_error("error: can't open pipe");
  fcntl(_pipe_from_cgi_fd[PIPE_READ], F_SETFL, O_NONBLOCK, FD_CLOEXEC);
  fcntl(_pipe_from_cgi_fd[PIPE_WRITE], F_SETFL, O_NONBLOCK, FD_CLOEXEC);
  _is_read_pipe_from_cgi_closed =false;

  if (pipe(_pipe_to_cgi_fd) != 0) throw std::runtime_error("error: can't open pipe");
  fcntl(_pipe_to_cgi_fd[PIPE_READ], F_SETFL, O_NONBLOCK, FD_CLOEXEC);
  fcntl(_pipe_to_cgi_fd[PIPE_WRITE], F_SETFL, O_NONBLOCK, FD_CLOEXEC);
  _is_write_pipe_to_cgi_closed = false;
  std::cout << "set pipe from read :" << _pipe_from_cgi_fd[PIPE_READ] << std::endl;
  std::cout << "set pipe from write :"<< _pipe_from_cgi_fd[PIPE_WRITE] << std::endl;
  std::cout << "set pipe to read :"<< _pipe_to_cgi_fd[PIPE_READ] << std::endl;
  std::cout << "set pipe to write :"<< _pipe_to_cgi_fd[PIPE_WRITE] << std::endl;

}

void CgiHandler::setupCgiEnvp(const std::map<std::string, SessionBlock>::const_iterator& sbi, bool is_joined_session) {
  static const char*      methods[] = {GET, HEAD, POST, DELETE};
  std::stringstream       ss;

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

  if (is_joined_session) {
    const SessionBlock&     session_block = sbi->second;

    setenv("session_id", session_block.getId().c_str(), 1);
    setenv("username", session_block.getValue().c_str(), 1);
  }

  char current_dir[512];
  getcwd(current_dir, 512);
  std::string slash = "/";
  setenv("PATH_TRANSLATED", (current_dir + slash + _route_rule.getRoot() + _req.getLocation()).c_str(), 1);
  setenv("REQUEST_URI", (_route_rule.getRoot() + _req.getLocation() + "?" + _req.getQueries()).c_str(), 1);
  setenv("SERVER_NAME", _server_name.c_str(), 1);
  ss << _port;
  setenv("SERVER_PORT", ss.str().c_str(), 1);
  setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);

  if (!_req.getHeaderValue("x-secret-header-for-test").empty())
    setenv("HTTP_X_SECRET_HEADER_FOR_TEST", _req.getHeaderValue("x-secret-header-for-test").c_str(), 1);
}
int CgiHandler::execute(const std::map<std::string, SessionBlock>::const_iterator& sbi, bool is_joined_session) throw(std::runtime_error) {
  pid_t pid = fork();
  if (pid == -1) throw std::runtime_error("error: fork error!");
  if (pid == 0) {  // Child process
      close(_pipe_from_cgi_fd[PIPE_READ]);
      dup2(_pipe_from_cgi_fd[PIPE_WRITE], STDOUT_FILENO);
      close(_pipe_from_cgi_fd[PIPE_WRITE]);

      close(_pipe_to_cgi_fd[PIPE_WRITE]);
  
      dup2(_pipe_to_cgi_fd[PIPE_READ], STDIN_FILENO);
      close(_pipe_to_cgi_fd[PIPE_READ]);

      this->setupCgiEnvp(sbi, is_joined_session);
      char* path        = strdup(_route_rule.getCgiPath().c_str());
      char* script_name = strdup((_route_rule.getRoot() + _req.getLocation()).c_str());
      char* argv[3] = {path, script_name, NULL};
      extern char** environ;
      if (execve(path, argv, environ) == -1) {
        exit(EXIT_FAILURE);
      }
      return 0;  //  This part is actually not reached.
  } else {  // Parent process
      close(_pipe_from_cgi_fd[PIPE_WRITE]);
      close(_pipe_to_cgi_fd[PIPE_READ]);
  }
  _pid = pid;
  return pid;
}

void                      CgiHandler::setIsReadPipeFromCgiClosed(bool is_closed) { _is_read_pipe_from_cgi_closed = is_closed; }
void                      CgiHandler::setIsWritePipeToCgiClosed(bool is_closed) { _is_write_pipe_to_cgi_closed = is_closed; }

const int&                CgiHandler::getClientFd(void) const{ return _client_fd; }
const std::vector<char>&  CgiHandler::getBuf(void) const{ return _buf; }
const RouteRule&          CgiHandler::getRouteRule(void) const { return _route_rule; };

void  CgiHandler::addBuf(const char* buf, size_t size){ _buf.insert(_buf.end(), buf, buf + size); }
void  CgiHandler::closeReadPipe(void){ close(_pipe_from_cgi_fd[PIPE_READ]); _is_read_pipe_from_cgi_closed = true; }
void  CgiHandler::closeWritePipe(void){ close(_pipe_to_cgi_fd[PIPE_WRITE]); _is_write_pipe_to_cgi_closed = true; }

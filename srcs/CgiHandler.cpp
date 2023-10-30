#include <unistd.h>
#include <fcntl.h>
#include <cstdlib>
#include <sstream>
#include "CgiHandler.hpp"

CgiHandler::CgiHandler() {}

CgiHandler::CgiHandler(
    HttpRequest& req, RouteRule& route_rule) throw(std::runtime_error)
  : _req(req), _route_rule(route_rule) {
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

  setenv("REQUEST_METHOD", methods[_req.getMethod() - 1], 1);
  setenv("QUERY_STRING", _req.getQueries().c_str(), 1);
  std::string content_type = _req.getHeaderValue("content-type");

  setenv("CONTENT_TYPE", (content_type.empty() ? DEFAULT_CONTENT_TYPE : content_type.c_str()), 1);
  ss << _req.getContentLength();
  setenv("CONTENT_LENGTH", ss.str().c_str(), 1);

  setenv("SCRIPT_NAME", (_route_rule.getRoot() + _req.getLocation()).c_str(), 1);
  setenv("REQUEST_URI", "/path/to/script.cgi?key=value", 1);
  setenv("PATH_INFO", "/extra/path/info", 1);
  setenv("PATH_TRANSLATED", "/file/system/path/for/path_info", 1);
  setenv("SERVER_NAME", "example.com", 1);
  setenv("SERVER_PORT", "80", 1);
  setenv("SERVER_PROTOCOL", "HTTP/1.1", 1);
  setenv("HTTP_USER_AGENT", "Mozilla/5.0", 1);
  setenv("REMOTE_ADDR", "192.168.0.1", 1);
  setenv("REMOTE_PORT", "12345", 1);
}

int CgiHandler::execute(void) throw(std::runtime_error) {
  pid_t pid = fork();

  if (pid == -1) throw std::runtime_error("error: fork error!");

  if (pid == 0) {  // Child process
      close(_pipe_from_cgi_fd[0]);
      dup2(_pipe_from_cgi_fd[1], STDOUT_FILENO);
      close(_pipe_from_cgi_fd[1]);
      close(_pipe_to_cgi_fd[1]);
      dup2(_pipe_to_cgi_fd[0], STDIN_FILENO);
      close(_pipe_to_cgi_fd[0]);

      this->setupCgiEnvp();

      if (execve(cgiPath.c_str(), argv, ) == -1) {
        throw std::runtime_error("error: execve error!");
      }
  } else {  // Parent process
      close(pipefd[1]);  // Close write end

      char buffer[4096];
      ssize_t bytesRead;
      while ((bytesRead = read(pipefd[0], buffer, sizeof(buffer))) > 0) {
          std::cout.write(buffer, bytesRead);
      }
      close(pipefd[0]);

      waitpid(pid, nullptr, 0);  // Wait for child process to finish
}

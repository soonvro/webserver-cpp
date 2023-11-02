#include <fstream>
#include <dirent.h>
#include <sstream>
#include "HttpResponse.hpp"
#include "HttpDecoderEnums.h"

void                                      HttpResponse::readFile(const std::string& path){
  std::ifstream i(path);
  if (i.fail()){
      throw FileNotFoundException();
  }
  char buf[BUF_SIZE];
  while (true){
    i.read(buf, BUF_SIZE);
    _body.insert(_body.end(), buf, buf + i.gcount());
    if (i.eof()){
      break ;
    }
    if (i.fail()){
      throw FileNotFoundException();
    }
  }
  // @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  // _headers["Content-Type"]에 path의 확장자에 따라서 적절한 값을 넣어줘야함.
  _headers["Content-Type"] = "text/html";
}

void                                      HttpResponse::readDir(const std::string& path){
  DIR* dir = opendir((path).c_str());
  struct dirent* entry;
  if (dir == nullptr){
    throw FileNotFoundException();
  }
  std::string br = "<br />";
  while ((entry = readdir(dir)) != nullptr) {
    _body.insert(_body.end(), entry->d_name, entry->d_name + strlen(entry->d_name));
    _body.insert(_body.end(), br.begin(), br.end());

  }
}


HttpResponse::HttpResponse() : _http_major(1), _http_minor(1), _status(0), \
  _content_length(0), _is_chunked(false), _is_ready(false) , _is_cgi(false) {}

// Getters
const unsigned short&                     HttpResponse::getHttpMajor() const { return _http_major; }
const unsigned short&                     HttpResponse::getHttpMinor() const { return _http_minor; }
const unsigned short&                     HttpResponse::getStatus() const { return _status; }
const std::string&                        HttpResponse::getStatusMessage() const { return _status_message; }
const std::map<std::string, std::string>& HttpResponse::getHeader() const { return _headers; }
const unsigned long long&                 HttpResponse::getContentLength() const { return _content_length; }
const bool&                               HttpResponse::getIsChunked() const { return _is_chunked; }
const std::vector<char>&                  HttpResponse::getBody() const { return _body; }
const bool&                               HttpResponse::getIsReady() const { return _is_ready; }
const bool&                               HttpResponse::getIsCgi() const { return _is_cgi; }
const int&                                HttpResponse::getCgiPipeIn(void) const { return _cgi_handler.getReadPipeFromCgi(); }
CgiHandler&                               HttpResponse::getCgiHandler() { return _cgi_handler; }

// Setters
void                                      HttpResponse::setHttpMajor(unsigned short http_major) { _http_major = http_major; }
void                                      HttpResponse::setHttpMinor(unsigned short http_minor) { _http_minor = http_minor; }
void                                      HttpResponse::setStatus(unsigned short status) { _status = status; }
void                                      HttpResponse::setStatusMessage(const std::string& status_message) { _status_message = status_message; }
void                                      HttpResponse::setHeaders(const std::map<std::string, std::string>& headers) { _headers = headers; }
void                                      HttpResponse::setContentLength(unsigned long long content_length) { _content_length = content_length; }
void                                      HttpResponse::setIsChunked(bool is_chunked) { _is_chunked = is_chunked; }
void                                      HttpResponse::setBody(const std::vector<char>& body) { _body = body; }
void                                      HttpResponse::setIsReady(bool is_ready) { _is_ready = is_ready; }
void                                      HttpResponse::setIsCgi(bool is_cgi) { _is_cgi = is_cgi; }

void                                      HttpResponse::addContentLength(void) {
  std::stringstream ss;
  ss << _body.size();
  _headers["Content-Length"] = ss.str();
  _content_length = _body.size();
}

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
bool                                      HttpResponse::isDir(const std::string& location){
  struct stat stat_buf;
  if (stat(location.c_str(), &stat_buf) != 0)
    return false;
  return S_ISDIR(stat_buf.st_mode);
}

void                                      HttpResponse::publish(const HttpRequest& req, const RouteRule& rule) {
    const std::string& location = req.getLocation();
    _headers["Content-Type"] = "text/html";
    _headers["Connection"] = "keep-alive";
    _is_ready = true;
    try{
    if (!(rule.getAcceptedMethods() & (1 << req.getMethod())))
      _status = 403;
    else if (rule.getMaxClientBodySize() != 0 &&
        rule.getMaxClientBodySize() < req.getEntity().size())
      _status = 413;
    else if (rule.getRedirection().first) {
      _status = rule.getRedirection().first;
      if (300 <= _status && _status < 400)
        _headers["Location"] = rule.getRedirection().second;
      else
        _body.assign(rule.getRedirection().second.begin(), rule.getRedirection().second.end());
      addContentLength();
      return ;
    } else if (isDir(rule.getRoot() + location)) {
      if (rule.getIndexPage().size() && rule.getRoute() == location) {
        _status = 200;
        if (!(req.getMethod() & HPS::kHEAD))
            readFile(rule.getRoot() + "/" + rule.getIndexPage());
      } else if (rule.getAutoIndex() && rule.getRoute() == location){
        _status = 200;
        if (!(req.getMethod() & HPS::kHEAD))
            readDir(rule.getRoot() + location);
      } else{
        _status = 404;
      }
    }else{
      _status = 200;
      if (!(req.getMethod() & HPS::kHEAD))
        readFile(rule.getRoot() + location);
    }
    } catch (FileNotFoundException &e){
      std::cout << "requested url not found" << std::endl;
      std::cout << e.what() << std::endl;
      if (rule.hasErrorPage(_status)) {
        try{
            if (!(req.getMethod() & HPS::kHEAD))
            readFile(rule.getRoot() + "/" + rule.getErrorPage(_status));
        } catch (FileNotFoundException &e){
          std::cout << "configured error page not found" << std::endl;
          std::cout << e.what() << std::endl;
          publishError(404);
        }
      } else{
        publishError(404);
      }
    }
    addContentLength();
}

void                                      HttpResponse::publishError(int status){
  _status = status;
  std::stringstream ss;
  ss << _status;
  std::string body_str("<html><body><h1>" + ss.str() + " error!</h1></body></html>");
  //_body.assign(body_str.begin(), body_str.end()); HEAD 요청시 반환하지 않아야한다.
  _headers["Content-Type"] = "text/html";
  if (status == 400){
    _headers["Connection"] = "close";
  }else{
    _headers["Connection"] = "keep-alive";
  }
  _is_ready = true;
  addContentLength();
}

void                                      HttpResponse::setHeader(const std::string& key, const std::string& value){ _headers[key] = value; }

void HttpResponse::initializeCgiProcess(
    HttpRequest& req, RouteRule& rule, const std::string& server_name, const int& port, const int& client_fd) throw(std::runtime_error) {
  _cgi_handler = CgiHandler(req, rule, server_name, port, client_fd);
}

int HttpResponse::cgiExecute(void) throw(std::runtime_error) {
  return _cgi_handler.execute();
}

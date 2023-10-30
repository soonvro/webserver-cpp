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
  while ((entry = readdir(dir)) != nullptr) {
    _body.insert(_body.end(), entry->d_name, entry->d_name + strlen(entry->d_name));
    _body.push_back('\n');
  }
}


HttpResponse::HttpResponse() : _http_major(0), _http_minor(0), _status(0), _content_length(0), _is_chunked(false) {}

// Getters
const unsigned short&                     HttpResponse::getHttpMajor() const { return _http_major; }
const unsigned short&                     HttpResponse::getHttpMinor() const { return _http_minor; }
const unsigned short&                     HttpResponse::getStatus() const { return _status; }
const std::string&                        HttpResponse::getStatusMessage() const { return _status_message; }
const std::map<std::string, std::string>& HttpResponse::getHeader() const { return _headers; }
const unsigned long long&                 HttpResponse::getContentLength() const { return _content_length; }
const bool&                               HttpResponse::getIsChunked() const { return _is_chunked; }
const std::vector<char>&                  HttpResponse::getBody() const { return _body; }

// Setters
void                                      HttpResponse::setHttpMajor(unsigned short http_major) { _http_major = http_major; }
void                                      HttpResponse::setHttpMinor(unsigned short http_minor) { _http_minor = http_minor; }
void                                      HttpResponse::setStatus(unsigned short status) { _status = status; }
void                                      HttpResponse::setStatusMessage(const std::string& status_message) { _status_message = status_message; }
void                                      HttpResponse::setHeaders(const std::map<std::string, std::string>& headers) { _headers = headers; }
void                                      HttpResponse::setContentLength(unsigned long long content_length) { _content_length = content_length; }
void                                      HttpResponse::setIsChunked(bool is_chunked) { _is_chunked = is_chunked; }
void                                      HttpResponse::setBody(const std::vector<char>& body) { _body = body; }

void                                      HttpResponse::addContentLength(void) {
  std::stringstream ss;
  ss << _body.size();
  _headers["Content-Length"] = ss.str();
  _content_length = _body.size();
}

void                                      HttpResponse::publish(const HttpRequest& req, const RouteRule& rule) {
    const std::string& location = req.getLocation();
    _headers["Content-Type"] = "text/html";
    _headers["Connection"] = "keep-alive";
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
    } else if (location[location.size() - 1] == '/') {
      if (rule.getIndexPage().size()) {
        _status = 200;
      if (!(req.getMethod() & HPS::kHEAD))
          readFile(rule.getRoot() + "/" + rule.getIndexPage());
      } else if (rule.getAutoIndex()){
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
  _body.assign(body_str.begin(), body_str.end());
  _headers["Content-Type"] = "text/html";
  _headers["Connection"] = "keep-alive";
  addContentLength();
}

void                                      HttpResponse::setHeader(const std::string& key, const std::string& value){ _headers[key] = value; }
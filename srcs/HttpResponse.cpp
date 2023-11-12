#include <fstream>
#include <dirent.h>
#include <sstream>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <cstring>
#include <iterator>

#include "HttpResponse.hpp"
#include "Client.hpp"

HttpResponse::HttpResponse(const HttpRequest& req, const RouteRule& route_rule) : _http_major(1), _http_minor(1), _status(0), \
  _content_length(0), _is_chunked(false), _is_ready(false) , _is_cgi(false), _cgi_handler(req, route_rule), _method(HPS::kHEAD), _entity_idx(0) {
    this->initContentTypes();
    _body.reserve(RESPONSE_BUF_SIZE);
  }

#define BUF_SIZE 4096

void HttpResponse::initContentTypes(void) {
  const char* extensions[] = {
    ".html", ".css", ".js", ".png", ".jpg",
    ".jpeg", ".gif", ".json", ".pdf", ".zip",
    ".tar", ".gz", ".mp4", ".mp3", ".avi",
    ".mpeg", ".wav", ".ogg", ".xml", ".txt"};
  const char* content_types[] = {
    "text/html", "text/css", "application/javascript", "image/png", "image/jpeg",
    "image/jpeg", "image/gif", "application/json", "application/pdf", "application/zip",
    "application/x-tar", "application/gzip", "video/mp4", "audio/mpeg", "video/x-msvideo",
    "video/mpeg", "audio/x-wav", "audio/ogg", "text/xml", "text/plain"};
  for (unsigned int i = 0; i < (sizeof(extensions)/sizeof(extensions[0])); i++) {
    _contentTypes[extensions[i]] = content_types[i];
  }
}

void                                      HttpResponse::readFile(const std::string& path){
  std::ifstream i(path.c_str());
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
  _headers["Content-Type"] = (path.rfind('.') != std::string::npos ? _contentTypes[path.substr(path.rfind('.'))] : "text/html");
}

void                                      HttpResponse::readDir(const std::string& path){
  DIR* dir = opendir((path).c_str());
  struct dirent* entry;
  if (dir == NULL){
    throw FileNotFoundException();
  }
  std::string br = "<br />";
  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_name[0] == '.')
      continue ;
    _body.insert(_body.end(), entry->d_name, entry->d_name + strlen(entry->d_name));

    _body.insert(_body.end(), br.begin(), br.end());
  }
  closedir(dir);
}

void                                      HttpResponse::deleteFile(const std::string& path){
  if (unlink(path.c_str()) != 0){
    _status = 500;
  }
}


bool                                      HttpResponse::isDir(const std::string& location){
  struct stat stat_buf;
  if (stat(location.c_str(), &stat_buf) != 0)
    return false;
  return S_ISDIR(stat_buf.st_mode);
}

void                                      HttpResponse::publish(const HttpRequest& req, const RouteRule* rule, const Client& client) {
    const std::string& location = req.getLocation();
    _method = req.getMethod();
    if (!rule){
      publishError(404, 0, _method);
      return ;
    }
    if (rule->getIsCgi()){
      //cgi set
      initializeCgiProcess(req, *rule, req.getHost(), client.getPort(), client.getClientFd());
      _is_cgi = true;
      return ;
    }
    const std::string  suffix_of_location(location.substr(rule->getRoute().size(), location.size() - rule->getRoute().size()));
    _headers["Content-Type"] = "text/html";
    _headers["Connection"] = "keep-alive";
    _is_ready = true;
    try{
        if (!(rule->getAcceptedMethods() & (1 << _method))) {
          publishError(405, rule, _method);
        } else if (rule->getMaxClientBodySize() != 0 &&
                rule->getMaxClientBodySize() < req.getEntity().size()) {
          publishError(413, rule, _method);
        } else if (rule->getRedirection().first) {
          _status = rule->getRedirection().first;
          if (300 <= _status && _status < 400) {
            _headers["Location"] = rule->getRedirection().second;
          }
          addContentLength();
          return ;
        } else if (isDir(rule->getRoot() + suffix_of_location)) {
            _status = 200;
          if (_method == HPS::kDELETE){
            deleteFile(rule->getRoot() + suffix_of_location);
          } else if (rule->getIndexPage().size() &&
              (rule->getRoute() == location || rule->getRoute() + '/' == location)) {  // index page event
            if (_method != HPS::kHEAD)
              readFile(rule->getRoot() + "/" + rule->getIndexPage());
          } else if (rule->getAutoIndex() &&
                    (rule->getRoute() == location || rule->getRoute() + '/' == location)) {  // index page event
            if (_method != HPS::kHEAD)
              readDir(rule->getRoot() + suffix_of_location);
          } else{
            publishError(404, rule, _method);
          }
        }else{
          _status = 200;
          if (_method == HPS::kDELETE){
            deleteFile(rule->getRoot() + suffix_of_location);
          } else if (_method != HPS::kHEAD)
            readFile(rule->getRoot() + suffix_of_location);
        }
    } catch (FileNotFoundException &e){
      std::cout << "requested url not found" << std::endl;
      std::cout << e.what() << std::endl;
      publishError(404, rule, _method);
    }
    addContentLength();
}

void                                      HttpResponse::publishCgi(const std::vector<char>::const_iterator& begin, const std::vector<char>::const_iterator& end,  const RouteRule& rule, enum HPS::Method method){
  std::string key;
  std::string value;
  std::vector<char>::const_iterator it = begin;
  std::vector<char>::const_iterator start = begin;
  while (start != end) {
    if (*it == ':'){
      key = std::string(start, it);
      start = it + 1;
    }else if (*it == '\n'){
      if (*(start - 1) != ':' || *(start - 1) == '\n') { ++it; break ; }
      if (*(it - 1) == '\r') value = std::string(start, it - 1);
      else value = std::string(start, it);
      setHeader(key, value);
      start = it + 1;
    }
    ++it;
  }
  setBody(it, end);
  _headers["Content-Type"] = "text/html";
  _headers["Connection"] = "keep-alive";
  if (_headers.find("Location") != _headers.end()){
    const std::string& location = _headers.find("Location")->second;
    if (location[0] == '/'){
      _is_ready = false;
      throw LocalReDirException();
    }else{
      _status = 302;
      setStatusMessage("Found");
    }
  } else if (_headers.find("Content-type") != _headers.end() || _headers.find("Content-Type") != _headers.end()) {
    _status = 200;
    setStatusMessage("OK");
  } else {
    _status = 500;
    publishError(500, &rule, method);
  }
  _is_ready = true;
  addContentLength();
}

void                                      HttpResponse::publishError(int status, const RouteRule* rule, enum HPS::Method method){
  _status = status;
  if (!rule || !rule->hasErrorPage(status)){
    std::stringstream ss;
    ss << _status;
    if (method != HPS::kHEAD) {
      std::string body_str("<html><body><h1>" + ss.str() + " error!</h1></body></html>");
      _body.assign(body_str.begin(), body_str.end());
    }
    _headers["Content-Type"] = "text/html";
    _headers["Connection"] = "keep-alive";
    _is_ready = true;
    addContentLength();
    return ;
  }
  try{
    if (method != HPS::kHEAD)
      readFile(rule->getRoot() + "/" + rule->getErrorPage(status));
  } catch (FileNotFoundException &e){
    std::cout << "configured error page not found" << std::endl;
    std::cout << e.what() << std::endl;
    publishError(404, 0, method);
  }
}

void                                      HttpResponse::initializeCgiProcess(
  const HttpRequest& req, const RouteRule& rule, const std::string& server_name, const int& port, const int& client_fd) throw(std::runtime_error) {
  _cgi_handler = CgiHandler(req, rule, server_name, port, client_fd);
}

int                                       HttpResponse::cgiExecute(void) throw(std::runtime_error) {
  return _cgi_handler.execute();
}

void                                      HttpResponse::addContentLength(void) {
  std::stringstream ss;
  ss << _body.size();
  _headers["Content-Length"] = ss.str();
  _content_length = _body.size();
}

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
HPS::Method                               HttpResponse::getMethod(void) const { return _method; }
const int&                                HttpResponse::getEntityIdx(void) const { return _entity_idx; };

// Setters
void                                      HttpResponse::setHttpMajor(unsigned short http_major) { _http_major = http_major; }
void                                      HttpResponse::setHttpMinor(unsigned short http_minor) { _http_minor = http_minor; }
void                                      HttpResponse::setStatus(unsigned short status) { _status = status; }
void                                      HttpResponse::setStatusMessage(const std::string& status_message) { _status_message = status_message; }
void                                      HttpResponse::setHeaders(const std::map<std::string, std::string>& headers) { _headers = headers; }
void                                      HttpResponse::setContentLength(unsigned long long content_length) { _content_length = content_length; }
void                                      HttpResponse::setIsChunked(bool is_chunked) { _is_chunked = is_chunked; }
void                                      HttpResponse::setBody(const std::vector<char>::const_iterator& it_begin, const std::vector<char>::const_iterator& it_end) { _body.insert(_body.end(), it_begin, it_end); }
void                                      HttpResponse::setIsReady(bool is_ready) { _is_ready = is_ready; }
void                                      HttpResponse::setIsCgi(bool is_cgi) { _is_cgi = is_cgi; }
void                                      HttpResponse::setHeader(const std::string& key, const std::string& value){ _headers[key] = value; }
void                                      HttpResponse::setEntityIdx(int entity_idx) { _entity_idx = entity_idx; }

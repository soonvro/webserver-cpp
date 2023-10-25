#include "HttpResponse.hpp"

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

// bool                                      HttpResponse::publish(const HttpRequest& req) {
//   if (req.getHeaderArrived() && req.getEntityArrived()) {
//     _http_major = req.getHttpMajor();
//     _http_minor = req.getHttpMinor();

//     // location
//     const std::string&  loc = req.getLocation();
//     std::ifstream       ifs;
//     std::stringstream   ss("");

//     if (loc == "/") { // root
//       ifs.open("index.html");
//       if (ifs.fail()) throw std::runtime_error("can't read file."); // 오류 처리 생각해봐야 함. 50x ?

//       ss << ifs.rdbuf();
      
//       char  c;
//       while (ss.get(c)) { _body.push_back(c); }
//       ss.str(""); // stringstream 초기화

//       _content_length = _body.size();
//       ss << _content_length; // buff size

//       _status = 200;
//       _status_message = "OK";
//       _headers["Content-Type"] = "text/html";
//       _headers["Content-Length"] = ss.str();
//     } else {
//       ifs.open("404.html");
//       if (ifs.fail()) throw std::runtime_error("can't read file."); // 오류 처리 생각해봐야 함. 50x ?

//       ss << ifs.rdbuf();
      
//       char  c;
//       while (ss.get(c)) { _body.push_back(c); }
//       ss.str(""); // stringstream 초기화

//       _content_length = _body.size();
//       ss << _content_length; // buff size

//       _status = 400;
//       _status_message = "Bad Request";
//       _headers["Content-Type"] = "text/html";
//       _headers["Content-Length"] = ss.str();
//     }
//     return true;
//   }
//     return false;
// }
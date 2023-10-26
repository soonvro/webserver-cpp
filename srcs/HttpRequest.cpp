#include <sstream>
#include "HttpRequest.hpp"

/******************************************************************************/
/*                                   Pubilc                                   */
/******************************************************************************/

HttpRequest::HttpRequest() 
  : _h_field(kHeaderNo), _has_host(false),
    _is_chunked(false), _is_connection_keep_alive(false),
  _is_connection_close(false), _is_content_length(false) {}

HttpRequest::~HttpRequest() {}

// Getters
const HPS::Method&                        HttpRequest::getMethod() const { return _method; }
const std::string&                        HttpRequest::getHost() const { return _host; }
const std::string&                        HttpRequest::getLocation() const { return _location; }
const std::string&                        HttpRequest::getQueries() const { return _queries; }
const unsigned short&                     HttpRequest::getHttpMajor() const { return _http_major; }
const unsigned short&                     HttpRequest::getHttpMinor() const { return _http_minor; }
const std::map<std::string, std::string>& HttpRequest::getHeaders() const { return _headers; }
const unsigned long long&                 HttpRequest::getContentLength() const { return _content_length; }
const bool&                               HttpRequest::getIsChunked() const { return _is_chunked; }
const std::vector<char>                   HttpRequest::getEntity() const { return _entity; }
const bool&                               HttpRequest::getHeaderArrived() const { return _header_arrived; }
const bool&                               HttpRequest::getEntityArrived() const { return _entity_arrived; }

/******************************************************************************/
/*                                   Private                                  */
/******************************************************************************/

bool HttpRequest::isStrCase(const char* lhs_start, unsigned int lhs_len,
                            const char* rhs) {
  if (!lhs_start || !rhs) return false;
  unsigned int i = 0;
  char c;
  while (i < lhs_len && *rhs) {
    c = static_cast<char>(std::tolower(lhs_start[i]));
    if (c != *rhs) return false;
    i++;
    rhs++;
  }
  if (i < lhs_len || *rhs) return false;
  return true;
}

bool HttpRequest::parseUrl(
    HttpDecoder* hd, const char *at, unsigned int len) {
  (void)hd;
  unsigned int idx = 0;
  HPS::DecoderState state = HPS::kUrlStart;
  const char* p_prev = NULL;
  std::stringstream ss;
  while (idx < len) {
    switch (state) {
      case HPS::kUrlStart:
        if ((*at) == '/') {
          p_prev = at;
          state = HPS::kUrlPath;
        }
        else if ((*at) == 'h' || (*at) == 'H') state = HPS::kUrlH;
        else return false;
        break;
      case HPS::kUrlH:
        at += (sizeof("ttp://") - 1);
        idx += (sizeof("ttp://") - 1);
        state = HPS::kUrlHostStart;
        continue;
      case HPS::kUrlHostStart:
        p_prev = at;
        state = HPS::kUrlHost;
        break;
      case HPS::kUrlHost:
        if ((*at) != '/' && (*at) != '?' && (*at) != ':' &&
            (idx != len - 1)) break;
        _host.assign(
            p_prev,
            static_cast<unsigned int>(at - p_prev + (idx == len - 1)));
        p_prev = NULL;
        if ((*at) == '/') {
          p_prev = at;
          state = HPS::kUrlPath;
        }
        else if ((*at) == '?') state = HPS::kUrlQuery;
        else if ((*at) == ':') state = HPS::kUrlPort;
        break;
      case HPS::kUrlPort:
        if (!p_prev) p_prev = at;
        if ((*at) != '/' && (*at) != '?'&&
            (idx != len - 1)) break;
        ss << std::string(
            p_prev,
            static_cast<unsigned int>(at - p_prev + (idx == len - 1)));
        ss >> _port;
        p_prev = NULL;
        if ((*at) == '/') {
          p_prev = at;
          state = HPS::kUrlPath;
        }
        else if ((*at) == '?') state = HPS::kUrlQuery;
        break;
      case HPS::kUrlPath:
        if ((*at) != '?'&&
            (idx != len - 1)) break;
        _location.assign(
            p_prev,
            static_cast<unsigned int>(at - p_prev + (idx == len - 1)));
        p_prev = NULL;
        if ((*at) == '?') state = HPS::kUrlQuery;
        break;
      case HPS::kUrlQuery:
        if (!p_prev) p_prev = at;
        if (idx + 1 == len) {
          _queries.assign(
            p_prev,
            static_cast<unsigned int>(at - p_prev + (idx == len - 1)));
          break;
        }
        break;
      default:
        return false;
    }
    at++;
    idx++;
  }
  return true;
}

bool HttpRequest::saveRquestData(
    HttpDecoder* hd) {
  if (!_has_host) return false;
  _method         = hd->_method;
  _http_major     = hd->_http_major;
  _http_minor     = hd->_http_minor;
  _content_length = hd->_content_length;
  _is_chunked               = (hd->_flag & HPS::kFlagChunked);
  _is_connection_keep_alive = (hd->_flag & HPS::kFlagConnectionKeepAlive);
  _is_connection_close      = (hd->_flag & HPS::kFlagConnectionClose);
  _is_content_length        = (hd->_flag & HPS::kFlagContentlength);
  return true;
}

bool HttpRequest::recognizeHeaderField(
    HttpDecoder* hd, const char *at, unsigned int len) {
  (void)hd;
  if (this->isStrCase(at, len, "host")) {
    if (_has_host) return false;
    _has_host = true;
    _h_field = kHeaderHost;
  }
  else {
    _last_headers_key.assign(at, len);
    _h_field = kHeaderNomal;
  }
  return true;
}

bool HttpRequest::parseHeaderValue(
    HttpDecoder* hd, const char *at, unsigned int len) {
  (void)hd;
  switch (_h_field) {
    case kHeaderHost:
      if (!_host.empty()) break;
      _host.assign(at, len);
      break;
    case kHeaderNomal:
      if (_headers.find(_last_headers_key) == _headers.end()) {
        _headers.insert(
            std::pair<std::string, std::string>
            (_last_headers_key, std::string(at, len)));
      }
      else {
        _headers[_last_headers_key] += (", " + std::string(at, len));
      }
      break;
    default:
      return false;
  }
  return true;
}

int HttpRequest::settingContent(const std::vector<char>& buf) {
  size_t i = 0;

  if (_entity.size() == _content_length) {
    _entity_arrived = true;
    return i;
  }

  for (; i < buf.size(); ++i) {
    if (_entity.size() == _content_length) {
      _entity_arrived = true;
      break ;
    }
    _entity.push_back(buf[i]);
  }

  return i;
}

/*  call back example

bool on_message_begin(HttpDecoder* hd) {
  return 0; 
}
static bool sOnMessageBegin(HttpDecoder* hd){
  HttpRequest* pthis = static_cast<HttpRequest*>(hd->_data);
  hd = 0;
  return pthis->on_message_begin(hd);
}

*/

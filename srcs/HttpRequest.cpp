#include <sstream>
#include <cctype>
#include <algorithm>
#include <iterator>
#include "HttpRequest.hpp"


/******************************************************************************/
/*                                   Pubilc                                   */
/******************************************************************************/

HttpRequest::HttpRequest()
  : _method(HPS::kHEAD), _http_major(0),
  _http_minor(0), _port(0), _h_field(kHeaderNo),
  _content_length(0), _chunked_block_length(0), _chunked_inserted_size(0), _chunked_idx(0),
  _is_host_header_comein(false), _is_chunked(false),
  _is_connection_keep_alive(false), _is_connection_close(false),
  _is_content_length(false), _has_chunked_len(false),
  _header_arrived(false), _entity_arrived(false) {
    _entity.reserve(1048576);
    _chunked_buf.reserve(1048576);
  }

HttpRequest::~HttpRequest() {}

HttpRequest::HttpRequest(const HttpRequest& other) {
  *this = other;
}

HttpRequest& HttpRequest::operator=(const HttpRequest& other){
  if (this == &other)
    return *this;
  _method = other._method;
  _http_major = other._http_major;
  _http_minor = other._http_minor;
  _host = other._host;
  _location = other._location;
  _port = other._port;
  _queries = other._queries;
  _h_field = other._h_field;
  _last_headers_key = other._last_headers_key;
  _headers = other._headers;
  _content_length = other._content_length;
  _chunked_block_length = other._chunked_block_length;
  _chunked_inserted_size = other._chunked_inserted_size;
  _chunked_idx = other._chunked_idx;
  _is_host_header_comein = other._is_host_header_comein;
  _is_chunked = other._is_chunked;
  _is_connection_keep_alive = other._is_connection_keep_alive;
  _is_connection_close = other._is_connection_close;
  _is_content_length = other._is_content_length;
  _has_chunked_len = other._has_chunked_len;
  _entity = other._entity;
  _chunked_buf = other._chunked_buf;
  _header_arrived = other._header_arrived;
  _entity_arrived = other._entity_arrived;
  return *this;
}

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
const std::vector<char>&                  HttpRequest::getEntity() const { return _entity; }
const bool&                               HttpRequest::getHeaderArrived() const { return _header_arrived; }
const bool&                               HttpRequest::getEntityArrived() const { return _entity_arrived; }
const std::string                         HttpRequest::getHeaderValue(std::string h_field) const { 
  std::map<std::string, std::string>::const_iterator iter = _headers.find(h_field);
  if (iter != _headers.end())
    return iter->second;
  return std::string();
}

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

bool HttpRequest::isOnlySlash(std::string& s) {
  for (std::string::iterator iter = s.begin(); iter != s.end(); iter++) {
    if (*iter != '/') return false;
  }
  return true;
}

unsigned char HttpRequest::toLower(unsigned char c) { return tolower(c); }

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
          if (idx == len - 1) _location.assign(p_prev, 1);
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
          if (idx == len - 1) _location.assign(p_prev, 1);
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
          if (idx == len - 1) _location.assign(p_prev, 1);
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
  if (!_is_host_header_comein) return false;
  _method         = hd->_method;
  _http_major     = hd->_http_major;
  _http_minor     = hd->_http_minor;
  _content_length = hd->_content_length;
  _is_chunked               = (hd->_flag & HPS::kFlagChunked);
  _is_connection_keep_alive = (hd->_flag & HPS::kFlagConnectionKeepAlive);
  _is_connection_close      = (hd->_flag & HPS::kFlagConnectionClose);
  _is_content_length        = (hd->_flag & HPS::kFlagContentlength);

  if (!this->isOnlySlash(_location)) {
    std::string::iterator new_end = _location.end();
    for (; (new_end != _location.begin() && *(new_end - 1) == '/') ; new_end--);
    _location = std::string(_location.begin(), new_end);
  }

  if (hd->_method == HPS::kPOST &&
      !(hd->_flag & HPS::kFlagContentlength) &&
      !(hd->_flag & HPS::kFlagChunked)) {
    return false;
  }
  return true;
}

bool HttpRequest::recognizeHeaderField(
    HttpDecoder* hd, const char *at, unsigned int len) {
  (void)hd;
  if (this->isStrCase(at, len, "host")) {
    _h_field = kHeaderHost;
  }
  else {
    _last_headers_key.assign(at, len);
    std::transform(_last_headers_key.begin(), _last_headers_key.end(), _last_headers_key.begin(), HttpRequest::toLower);
    _h_field = kHeaderNomal;
  }
  return true;
}

bool HttpRequest::parseHeaderValue(
    HttpDecoder* hd, const char *at, unsigned int len) {
  (void)hd;
  unsigned int host_end = 0;
  switch (_h_field) {
    case kHeaderHost:
      if (_is_host_header_comein) return false;  // multiple host header
      _is_host_header_comein = true;
      if (!_host.empty()) return true;  // request target has host
      for (; (host_end < len && at[host_end] != ':'); ++host_end);
      _host.assign(at, host_end);
      return true;
    case kHeaderNomal:
      if (_headers.find(_last_headers_key) == _headers.end()) {
        _headers.insert(
            std::pair<std::string, std::string>
            (_last_headers_key, std::string(at, len)));
      }
      else {
        _headers[_last_headers_key] += (", " + std::string(at, len));
      }
      return true;
    default:
      return false;
  }
  return false;
}

void  HttpRequest::chunkedLength(const std::vector<char>& buf, size_t& i) {
  size_t  j = i;
  while (i < buf.size() && buf[i] != '\n') ++i;

  if ((i > 0 && j == i - 1 && buf[i - 1] == '\r') || j == i) {
    _chunked_block_length = -1;
    return ;
  }

  if (i < buf.size() && buf[i] == '\n') {
    _chunked_block_length = 0;
    for (; j < (i - (buf[i - 1] == '\r')); ++j) {
        if ('0' <= buf[j] && buf[j] <= '9') _chunked_block_length = _chunked_block_length * 16 + buf[j] - '0';
        else if ('A' <= buf[j] && buf[j] <= 'F') _chunked_block_length = _chunked_block_length * 16 + buf[j] - 'A' + 10;
        else if ('a' <= buf[j] && buf[j] <= 'f') _chunked_block_length = _chunked_block_length * 16 + buf[j] - 'a' + 10;
        else {
          _chunked_block_length = -1;
          break ;
        }
    }
    _has_chunked_len = true;
    ++i;
  }
}

void  HttpRequest::chunkedSetting(const std::vector<char>& buf, size_t& idx) {
  //copy
  _chunked_buf.insert(_chunked_buf.end(), buf.begin(), buf.end());
  
  size_t chunked_start = _chunked_idx;
  while (_chunked_idx < _chunked_buf.size()){
    if (_has_chunked_len == false){
      chunkedLength(_chunked_buf, _chunked_idx);
      if (_chunked_block_length > 0) _content_length += static_cast<unsigned long long>(_chunked_block_length);
      if (_chunked_block_length == 0) _entity_arrived = true;
      if (_chunked_block_length < 0) 
        throw ChunkedException();
      idx = _chunked_idx - chunked_start;
      if (_entity_arrived) break;
    } else {
      if (_chunked_buf.size() >= _chunked_idx + _chunked_block_length + 2) {
        if (_chunked_buf[_chunked_idx + _chunked_block_length] == '\r' && _chunked_buf[_chunked_idx + _chunked_block_length + 1] == '\n') {
          _entity.insert(_entity.end(), _chunked_buf.begin() + _chunked_idx, _chunked_buf.begin() + _chunked_idx + _chunked_block_length);
          _chunked_idx += _chunked_block_length + 2;
          idx += _chunked_block_length + 2 - chunked_start;
        } else {
          throw ChunkedException();
        }
        _has_chunked_len = false;
      } else {
        idx = buf.size();
        break;
      }
    }
  }
}

int HttpRequest::settingContent(const std::vector<char>& buf) {
  size_t i = 0;

  if (_is_chunked) {
    if (buf.size()) chunkedSetting(buf, i);
  } else {
    std::vector<char>::const_iterator iter = buf.begin();
    std::advance(iter, i);
    _entity.insert(_entity.end(), iter, iter + _content_length - _entity.size());
    if (_entity.size() == _content_length)
      _entity_arrived = true;
  }
  return i;
}

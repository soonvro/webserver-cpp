#include <sstream>
#include <cctype>
#include <algorithm>
#include "HttpRequest.hpp"

/******************************************************************************/
/*                                   Pubilc                                   */
/******************************************************************************/

HttpRequest::HttpRequest() 
  : _h_field(kHeaderNo),_content_length(0), _chunked_block_length(0),
  _has_host(false), _is_chunked(false), _is_connection_keep_alive(false),
  _is_connection_close(false), _is_content_length(false), _has_chunked_len(false),
  _header_arrived(false), _entity_arrived(false) {}

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
  if (!_has_host) return false;
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
      if (!_host.empty()) break;
      for (; (host_end < len && at[host_end] != ':'); ++host_end);
      _host.assign(at, host_end);
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

void  HttpRequest::chunkedLength(const std::vector<char>& buf, size_t& i) {
  size_t  j = i;
  while (i < buf.size() && buf[i] != '\n') ++i;

  if (j == i) {
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

void  HttpRequest::chunkedSetting(const std::vector<char>& buf, size_t& i) {
  if (buf.size() <= i) return ;

  if (!_has_chunked_len) {
    chunkedLength(buf, i);
    if (_chunked_block_length > 0) _content_length += static_cast<unsigned long long>(_chunked_block_length);
    else if (_has_chunked_len && _chunked_block_length == 0) _entity_arrived = true;
    else if (_chunked_block_length < 0) throw ChunkedException();
  }
  for (; i < buf.size(); ++i) {
    if (_chunked_block_length == 0) {
      if (_has_chunked_len && i < buf.size()) {
        if (buf[i] == '\r' && i < buf.size() - 1) ++i;
        if (buf[i] == '\n') {
          _has_chunked_len = false; ++i;
          chunkedSetting(buf, i);
        }
        else if (buf[i] != '\r') throw ChunkedException();
      }
      break ;
    }
    _entity.push_back(buf[i]);
    --_chunked_block_length;
  }
}

int HttpRequest::settingContent(const std::vector<char>& buf) {
  size_t i = 0;

  if (_is_chunked) {
    if (buf.size()) chunkedSetting(buf, i);
  } else {
    for (; i < buf.size(); ++i) {
      if (_entity.size() == _content_length) {
        _entity_arrived = true;
        break ;
      }
      _entity.push_back(buf[i]);
    }

    if (_entity.size() == _content_length)
      _entity_arrived = true;
  }
  return i;
}

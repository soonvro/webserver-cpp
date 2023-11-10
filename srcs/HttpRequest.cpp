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
  _content_length(0), _chunked_block_size(0), _chunked_block_length(0),
  _is_host_header_comein(false), _is_chunked(false),
  _is_connection_keep_alive(false), _is_connection_close(false),
  _is_content_length(false), _has_chunked_len(false), _pre_cr(false),
  _header_arrived(false), _entity_arrived(false) {
  _entity.reserve(REQUEST_BUF_SIZE);
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
  _chunked_block_size = other._chunked_block_size;
  _chunked_block_length = other._chunked_block_length;
  _is_host_header_comein = other._is_host_header_comein;
  _is_chunked = other._is_chunked;
  _is_connection_keep_alive = other._is_connection_keep_alive;
  _is_connection_close = other._is_connection_close;
  _is_content_length = other._is_content_length;
  _has_chunked_len = other._has_chunked_len;
  _pre_cr = other._pre_cr;
  _entity.reserve(other._entity.capacity());
  _entity = other._entity;
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

// Setters
void HttpRequest::setQueries(const std::string& queries) { _queries = queries; }
void HttpRequest::setLocation(const std::string& location) { _location = location; }

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

void HttpRequest::decode_location(void) {
  std::string decoded_loc;
  decoded_loc.reserve(_location.length());

  for (std::size_t i = 0; i < _location.length(); ++i) {
    if (_location[i] == '%' && i + 2 < _location.length()) {
      std::stringstream ss;
      std::string hex = _location.substr(i + 1, 2);
      int value;
      ss << std::hex << hex;
      ss >> value;
      decoded_loc += static_cast<char>(value);
      i += 2;  // move to after %XX
    } else if (_location[i] == '+') {
      decoded_loc += ' ';
    } else {
      decoded_loc += _location[i];
    }
  }
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

  this->decode_location();
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

void  HttpRequest::chunkedLength(std::vector<char>::const_iterator& start, std::vector<char>::const_iterator& end) {
  for (; start != end; ++start) {

    if (*start == '\n') {
      _has_chunked_len = true;
      _pre_cr = false;
      _content_length += _chunked_block_size;
      _chunked_block_length = _chunked_block_size;
      ++start;
      break ;
    }

    if ('0' <= *start && *start <= '9') _chunked_block_size = _chunked_block_size * 16 + *start - '0';
    else if ('A' <= *start && *start <= 'F') _chunked_block_size = _chunked_block_size * 16 + *start - 'A' + 10;
    else if ('a' <= *start && *start <= 'f') _chunked_block_size = _chunked_block_size * 16 + *start - 'a' + 10;
    else if (!_pre_cr && *start == '\r') _pre_cr = true;
    else throw ChunkedException();
  }
}

void  HttpRequest::chunkedSetting(std::vector<char>::const_iterator& start, std::vector<char>::const_iterator& end) {
  while (0 < std::distance(start, end)) {
    if (_has_chunked_len == false) chunkedLength(start, end);
    else {
      if (_chunked_block_size == 0) break ;

      if (_chunked_block_length <= std::distance(start, end)) {
        if (_chunked_block_length == 0) {
          if (!_pre_cr && *start == '\r') {
            _pre_cr = true;
            ++start;
          }
          if (std::distance(start, end)) {
            if (*start == '\n') {
              _pre_cr = false;
              _has_chunked_len = false;
              _chunked_block_size = 0;
              ++start;
            } else throw ChunkedException();
          }
        } else if (_chunked_block_length) {
          _entity.insert(_entity.end(), start, start + _chunked_block_length);
          start += _chunked_block_length;
          _chunked_block_length = 0;
        }
      } else {
        size_t  size = std::distance(start, end);
        _entity.insert(_entity.end(), start, end);
        _chunked_block_length -= size;
        start += size;
      }
    }
  }
  if (_has_chunked_len && _chunked_block_size == 0) _entity_arrived = true;
}

int HttpRequest::settingContent(std::vector<char>::const_iterator start, std::vector<char>::const_iterator end) {
  int                               i = 0;
  std::vector<char>::const_iterator it = start;

  if (_is_chunked) {
    if (std::distance(start, end)) chunkedSetting(start, end);
    i = std::distance(it, start);
  } else {
    if (_content_length && std::distance(start, end)) _entity.insert(_entity.end(), start, end);
    i = std::distance(start, end);
    if (_entity.size() == _content_length)
      _entity_arrived = true;
  }
  return i;
}

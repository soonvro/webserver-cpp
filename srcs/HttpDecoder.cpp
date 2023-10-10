#include "HttpDecoder.h"

#include <cctype> //isalpha
#include <cstring> //memcmp, memset
#include <limits> // std::numeric_limits<unsigned long long>::max()
#include <stdexcept>

#define URL_LEN_MAX 2000

#define HPS_LITERAL_STRLEN(str) (sizeof(str) - 1)

#define HPS_IS_STR(lhs, rhs) \
  ((_buf_len >= _n_read + HPS_LITERAL_STRLEN(rhs)) && \
   (std::memcmp((lhs), (rhs), HPS_LITERAL_STRLEN(rhs)) == 0))

#define HPS_IS_VCHAR(c) ((c) >= 33 && (c) <= 126)
#define HPS_IS_TCHAR(c) \
  (std::isalnum(c) || \
   (c) == '!'  || (c) == '#' || (c) == '$' || (c) == '%' || (c) == '&' || \
   (c) == '\'' || (c) == '*' || (c) == '+' || (c) == '-' || (c) == '.' || \
   (c) == '^'  || (c) == '_' || (c) == '`' || (c) == '|' || (c) == '~')
#define HPS_IS_WHITESPACE(c) ((c) == ' ' || (c) == '\t')
#define HPS_IS_NEWLINE(p) (*(p) == '\n' || HPS_IS_STR((p), "\r\n"))

/******************************************************************************/
/*                                   Pubilc                                   */
/******************************************************************************/

HttpDecoder::HttpDecoder()
  : _method(HPS::kNotMethod), _uses_transfer_encoding(false),
    _content_length(0), _flag(0), _errno(HPS::kFailure), _data(NULL),
    _buf(NULL), _n_read(0), _state(HPS::kMethodStart) {
  std::memset(&_cb, 0, sizeof(HttpDecoderCallback));
}

HttpDecoder::~HttpDecoder() {}

void HttpDecoder::setCallback(
    t_http_cb      on_message_begin, t_http_data_cb on_url,
    t_http_data_cb on_status,        t_http_data_cb on_header_field,
    t_http_data_cb on_header_value,  t_http_cb      on_headers_complete,
    t_http_data_cb on_body,          t_http_cb      on_message_complete,
    t_http_cb      on_chunk_header,  t_http_cb      on_chunk_complete) {
  _cb.on_message_begin    = on_message_begin;
  _cb.on_url              = on_url;
  _cb.on_status           = on_status;
  _cb.on_header_field     = on_header_field;
  _cb.on_header_value     = on_header_value;
  _cb.on_headers_complete = on_headers_complete;
  _cb.on_body             = on_body;
  _cb.on_message_complete = on_message_complete;
  _cb.on_chunk_header     = on_chunk_header;
  _cb.on_chunk_complete   = on_chunk_complete;
}

void HttpDecoder::setDataSpace(void* data) { _data = data; }

unsigned int HttpDecoder::execute(const char* buf, const unsigned int len) {
  if (!buf) return 0;
  _buf     = buf;
  _buf_len = len;
  _p       = buf;
  _c       = *_p;
  const char* p_prev;
  while (_p != _buf + _buf_len) {
    if (_p > _buf + _buf_len)
      throw std::runtime_error("HttpDecoder accesses undefined memory");
    switch (_state) {
      case HPS::kMethodStart:
        if (HPS_IS_NEWLINE(_p)) {
          this->readNBytes(_c == '\n' ? 1 : 2);
        }
        _state = this->checkMethod();
        break;

      case HPS::kMethodEnd:
        if (_c == ' ') {
          this->readNBytes(1);
          _state = HPS::kUrlStart;
        }
        else _state = HPS::kDead;
        break;

      case HPS::kUrlStart:
        p_prev = _p;
        _state = this->checkUrl(p_prev);
        break;

      case HPS::kUrlEnd:
        if (_c == ' ') {
          if (_cb.on_url) {
            _cb.on_url(this, p_prev, static_cast<unsigned int>(_p - p_prev));
          }
          this->readNBytes(1);
          _state = HPS::kHttpVerStart;
        }
        else _state = HPS::kDead;
        break;

      case HPS::kHttpVerStart:
        _state = this->checkHttpVer();
        break;

      case HPS::kHttpVerEnd:
        if (HPS_IS_NEWLINE(_p)) {
          this->readNBytes(_c == '\n' ? 1 : 2);
          _state = HPS::kHeaderFieldStart;
        }
        else _state = HPS::kDead;
        break;

      case HPS::kHeaderFieldStart:
        if (HPS_IS_NEWLINE(_p)) {
          this->readNBytes(_c == '\n' ? 1 : 2);
          if (_cb.on_headers_complete) {
            _cb.on_headers_complete(this);
          }
          return _n_read;
        }
        p_prev = _p;
        while (HPS_IS_TCHAR(_c)) this->readNBytes(1);
        _state = HPS::kHeaderFieldEnd;
        break;

      case HPS::kHeaderConnection:
        _state = this->checkConnection();
        break;

      case HPS::kHeaderContentLength:
        if (_flag & HPS::kFlagChunked) _state = HPS::kDead;
        else _state = this->checkContentLength();
        break;

      case HPS::kHeaderTransferEncoding:
        _uses_transfer_encoding = true;
        _state = this->checkTransferEncoding();
        break;

      case HPS::kHeaderFieldEnd:
        if (_p != p_prev && _c == ':') {
          if (isStrNCaseInsensOfLhs(p_prev, _p, CONNECTION,
                                    HPS_LITERAL_STRLEN(CONNECTION))) {
            _state = HPS::kHeaderConnection;
          }
          else if (isStrNCaseInsensOfLhs(p_prev, _p, CONTENT_LENGTH,
                                    HPS_LITERAL_STRLEN(CONTENT_LENGTH))) {
            _state = HPS::kHeaderContentLength;
          }
          else if (isStrNCaseInsensOfLhs(p_prev, _p, TRANSFER_ENCODING,
                                    HPS_LITERAL_STRLEN(TRANSFER_ENCODING))) {
            _state = HPS::kHeaderTransferEncoding;
          }
          else {
            if (_cb.on_header_field) {
              _cb.on_header_field(this, p_prev, static_cast<unsigned int>(_p - p_prev));
            }
            _state = HPS::kHeaderValueStart;
          }
          this->readNBytes(1);
          while (HPS_IS_WHITESPACE(_c)) this->readNBytes(1);
          if (HPS_IS_NEWLINE(_p)) {
            _state = HPS::kDead;
            break;
          }
        }
        else _state = HPS::kDead;
        break;

      case HPS::kHeaderValueStart:
        p_prev = _p;
        while (HPS_IS_WHITESPACE(_c) || HPS_IS_VCHAR(_c)) this->readNBytes(1);
        if (HPS_IS_NEWLINE(_p)) _state = HPS::kHeaderValueEnd;
        else _state = HPS::kDead;
        break;

      case HPS::kHeaderValueEnd:
        if (_cb.on_header_value) {
          _cb.on_header_value(this, p_prev, static_cast<unsigned int>(_p - p_prev));
        }
        this->readNBytes(_c == '\n' ? 1 : 2);
        _state = HPS::kHeaderFieldStart;
        break;

      case HPS::kDead:
        _errno = HPS::kFailure;
        return 0;

      default:
        _state = HPS::kDead;
        break;
    }
  }
  _errno = HPS::kFailure;
  return 0;
}

/******************************************************************************/
/*                                   Private                                  */
/******************************************************************************/

HttpDecoder::HttpDecoder(const HttpDecoder& rhs) { (void)rhs; }
HttpDecoder& HttpDecoder::operator=(const HttpDecoder& rhs) {
  (void)rhs;
  return *this;
}

bool HttpDecoder::isStrNCaseInsensOfLhs(const char* lhs_start, const char* lhs_end,
                                        const char* rhs, unsigned int rhs_len) {
  if (!lhs_start || (lhs_start >= lhs_end) || !rhs) return false;
  if (lhs_end - lhs_start != rhs_len) return false;
  char c;
  while (lhs_start < lhs_end) {
    c = static_cast<char>(std::tolower(*lhs_start));
    if (c != *rhs) return false;
    lhs_start++;
    rhs++;
  }
  return true;
}

void HttpDecoder::readNBytes(unsigned int n) {
  if (_n_read + n > _buf_len) {
    _state = HPS::kDead;
    return;
  }
  _p += n;
  _n_read += n;
  _c = *_p;
  return;
}

bool HttpDecoder::isNormalUrlChar(const char c) {
  static const bool is_url_char[] = {
  // 0 nul    1 soh    2 stx    3 etx    4 eot    5 enq    6 ack    7 bel
        0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0,
  // 8 bs     9 ht    10 nl    11 vt    12 np    13 cr    14 so    15 si
        0    ,   0    ,   0    ,   0    ,    0   ,   0    ,   0    ,   0,
  //16 dle   17 dc1   18 dc2   19 dc3   20 dc4   21 nak   22 syn   23 etb
        0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0,
  //24 can   25 em    26 sub   27 esc   28 fs    29 gs    30 rs    31 us
        0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0    ,   0,
  //32 sp    33  !    34  "    35  #    36  $    37  %    38  &    39  '
        0    ,   1    ,   0    ,   0    ,   1    ,   1    ,   1    ,   1,
  //40  (    41  )    42  *    43  +    44  ,    45  -    46  .    47  /
        1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //48  0    49  1    50  2    51  3    52  4    53  5    54  6    55  7
        1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //56  8    57  9    58  :    59  ;    60  <    61  =    62  >    63  ?
        1    ,   1    ,   1    ,   1    ,   0    ,   1    ,   0    ,   0,
  //64  @    65  A    66  B    67  C    68  D    69  E    70  F    71  G
        1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //72  H    73  I    74  J    75  K    76  L    77  M    78  N    79  O
        1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //80  P    81  Q    82  R    83  S    84  T    85  U    86  V    87  W
        1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //88  X    89  Y    90  Z    91  [    92  \    93  ]    94  ^    95  _
        1    ,   1    ,   1    ,   0    ,   0    ,   0    ,   0    ,   1,
  //96  `    97  a    98  b    99  c   100  d   101  e   102  f   103  g
        0    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //04  h   105  i   106  j   107  k   108  l   109  m   110  n   111  o
        1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //12  p   113  q   114  r   115  s   116  t   117  u   118  v   119  w
        1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1    ,   1,
  //20  x   121  y   122  z   123  {   124  |   125  }   126  ~   127 del
        1    ,   1    ,   1    ,   0    ,   0    ,   0    ,   0    ,   0, };
  if (static_cast<int>(c) < 0) return false;
  return is_url_char[static_cast<int>(c)];
}

#define HPS_MATCH_METHOD(mth) \
do { \
  if (HPS_IS_STR(_p, (mth))) { \
    _method = HPS::k##mth; \
    this->readNBytes(HPS_LITERAL_STRLEN(mth)); \
    return HPS::kMethodEnd; \
  }\
} while(0)

enum HPS::DecoderState HttpDecoder::checkMethod(void) {
  HPS_MATCH_METHOD(GET);
  HPS_MATCH_METHOD(HEAD);
  HPS_MATCH_METHOD(POST);
  HPS_MATCH_METHOD(DELETE);
  return HPS::kDead;
}

// This check origin-form.
// This decoder only allows the "http" schema.
enum HPS::DecoderState HttpDecoder::checkUrl(const char* const& url_start) {
  while (true) {
    if (_p - url_start > URL_LEN_MAX) return HPS::kDead;
    switch (_state) {
      case HPS::kUrlStart:
        // if (std::isalpha(_c)) {
          // _state = HPS::kUrlSchema;
          // continue;
        // }
        if (_c != '/') return HPS::kDead;
        _state = HPS::kUrlPath;
        break;
      // case HPS::kUrlSchema:
        // if (std::isalpha(_c)) break;
        // if (!HPS_IS_STR(_p, "://")) return HPS::kDead;
        // strToLower(const_cast<char*>(url_start), const_cast<char*>(_p));
        // if (!HPS_IS_STR(url_start, "http"))  return HPS::kDead;
        // _p += (HPS_LITERAL_STRLEN("://") - 1);
        // _n_read += (HPS_LITERAL_STRLEN("://") - 1);
        // _state = HPS::kUrlHost;
        // break;
      // case HPS::kUrlHost:
        // if (_c == '/') _state = HPS::kUrlPath;
        // else if (_c == '?') _state = HPS::kUrlQuery;
        // else if (!this->isNormalUrlChar(_c)) return HPS::kDead;
        // break;
      case HPS::kUrlPath:
        if (_c == ' ') return HPS::kUrlEnd;
        if (_c == '?') _state = HPS::kUrlQuery;
        else if (!this->isNormalUrlChar(_c)) return HPS::kDead;
        break;
      case HPS::kUrlQuery:
        if (_c == ' ') return HPS::kUrlEnd;
        if (_c != '?' && !this->isNormalUrlChar(_c)) return HPS::kDead;
        break;
      default:
        return HPS::kDead;
    }
    this->readNBytes(1);
  }
  return HPS::kDead;
}

enum HPS::DecoderState HttpDecoder::checkHttpVer(void) {
  if (!HPS_IS_STR(_p, HTTP)) return HPS::kDead;
  this->readNBytes(HPS_LITERAL_STRLEN(HTTP));

  if (_c != '/') return HPS::kDead;
  this->readNBytes(1);

  if (!std::isdigit(_c)) return HPS::kDead;
  _http_major = static_cast<unsigned short>(_c) - static_cast<unsigned short>('0');
  this->readNBytes(1);

  if (_c != '.') return HPS::kDead;
  this->readNBytes(1);

  if (!std::isdigit(_c)) return HPS::kDead;
  _http_minor = static_cast<unsigned short>(_c) - static_cast<unsigned short>('0');
  this->readNBytes(1);
  return HPS::kHttpVerEnd;
}

enum HPS::DecoderState HttpDecoder::checkConnection(void) {
  const char* value_start;
  bool has_value = false;
  while (true) {
    if (HPS_IS_TCHAR(_c)) {
      if (!has_value) {
        has_value = true;
        value_start = _p;
      }
    }
    else if (_c == ',' || HPS_IS_WHITESPACE(_c) || HPS_IS_NEWLINE(_p)) {
      if (has_value) {
        has_value = false;
        if (this->isStrNCaseInsensOfLhs(
              value_start, _p, KEEP_ALIVE, HPS_LITERAL_STRLEN(KEEP_ALIVE))) {
          if (_flag & HPS::kFlagConnectionClose) return HPS::kDead;
          _flag |= HPS::kFlagConnectionKeepAlive;
        }
        if (this->isStrNCaseInsensOfLhs(
              value_start, _p, CLOSE, HPS_LITERAL_STRLEN(CLOSE))) {
          if (_flag & HPS::kFlagConnectionKeepAlive) return HPS::kDead;
          _flag |= HPS::kFlagConnectionClose;
        }
      }
      if (HPS_IS_NEWLINE(_p)) {
        this->readNBytes(_c == '\n' ? 1 : 2);
        return HPS::kHeaderFieldStart;
      }
    }
    else return HPS::kDead;
    this->readNBytes(1);
  }
}

enum HPS::DecoderState HttpDecoder::checkContentLength(void) {
  static unsigned long long ull_max = std::numeric_limits<unsigned long long>::max();

  if (_flag & HPS::kFlagContentlength) return HPS::kDead;
  _flag |= HPS::kFlagContentlength;

  bool         is_first_value = true;
  bool         has_value = false;
  unsigned int con_len = 0;
  while (true) {
    if (std::isdigit(_c)) {
      if (!has_value) has_value = true;
      if (con_len > ull_max / 10 || (con_len == ull_max && _c > '5')) {
        return HPS::kDead;
      }
      con_len = con_len * 10 +
        (static_cast<unsigned int>(_c) - static_cast<unsigned int>('0'));
    }
    else if (_c == ',' || HPS_IS_WHITESPACE(_c) || HPS_IS_NEWLINE(_p)) {
      if (has_value) {
        if (is_first_value) {
          _content_length = con_len;
          is_first_value = false;
        }
        else if (_content_length != con_len) return HPS::kDead;
        con_len = 0;
        has_value = false;
      }
      if (HPS_IS_NEWLINE(_p)) {
        this->readNBytes(_c == '\n' ? 1 : 2);
        return HPS::kHeaderFieldStart;
      }
    }
    else return HPS::kDead;
    this->readNBytes(1);
  }
}

enum HPS::DecoderState HttpDecoder::checkTransferEncoding(void) {
  const char* value_start;
  enum HPS::DecoderHeaderState h_state = HPS::kTransferStart;
  while (true) {
    switch (h_state) {
      case HPS::kTransferStart:
        if (HPS_IS_TCHAR(_c)) {
          value_start = _p;
          h_state = HPS::kTransferOnValue;
        }
        else if(!HPS_IS_WHITESPACE(_c) && _c != ',') return HPS::kDead;
        break;
      case HPS::kTransferNeedValue:
        if (HPS_IS_TCHAR(_c)) {
          value_start = _p;
          h_state = HPS::kTransferOnValue;
        }
        else if (HPS_IS_NEWLINE(_p)) {
          this->readNBytes(_c == '\n' ? 1 : 2);
          return HPS::kHeaderFieldStart;
        }
        else if (!HPS_IS_WHITESPACE(_c) && _c != ',') return HPS::kDead;
        break;
      case HPS::kTransferOnValue:
        if (_c == ',' || _c == ';' || HPS_IS_NEWLINE(_p) || HPS_IS_WHITESPACE(_c)) {
          if (isStrNCaseInsensOfLhs(value_start, _p, CHUNKED,
                                    HPS_LITERAL_STRLEN(CHUNKED))) {
            if (_flag & HPS::kFlagContentlength) return HPS::kDead;
            if (_flag & HPS::kFlagChunked) return HPS::kDead;
            _flag |= HPS::kFlagChunked;
          }
          if (_c == ',') h_state = HPS::kTransferNeedValue;
          else if (_c == ';') h_state = HPS::kTransferNeedParamL;
          else if (HPS_IS_WHITESPACE(_c)) h_state = HPS::kTransferEndValue;
          else if (HPS_IS_NEWLINE(_p)) {
            this->readNBytes(_c == '\n' ? 1 : 2);
            return HPS::kHeaderFieldStart;
          }
          else return HPS::kDead;
        }
        else if(!HPS_IS_TCHAR(_c)) return HPS::kDead;
        break;
      case HPS::kTransferEndValue:
        if (_c == ',') h_state = HPS::kTransferNeedValue;
        else if (_c == ';') h_state = HPS::kTransferNeedParamL;
        else if (HPS_IS_NEWLINE(_p)) {
          this->readNBytes(_c == '\n' ? 1 : 2);
          return HPS::kHeaderFieldStart;
        }
        else if (!HPS_IS_WHITESPACE(_c)) return HPS::kDead;
        break;
      case HPS::kTransferNeedParamL:
        if (HPS_IS_TCHAR(_c)) h_state = HPS::kTransferOnParamL;
        else if (HPS_IS_WHITESPACE(_c)) return HPS::kDead;
        break;
      case HPS::kTransferOnParamL:
        if (HPS_IS_WHITESPACE(_c)) h_state = HPS::kTransferEndParamL;
        else if (_c == '=') h_state = HPS::kTransferNeedParamR;
        else if(!HPS_IS_TCHAR(_c)) return HPS::kDead;
        break;
      case HPS::kTransferEndParamL:
        if (_c == '=') h_state = HPS::kTransferNeedParamR;
        else if (!HPS_IS_WHITESPACE(_c)) return HPS::kDead;
        break;
      case HPS::kTransferNeedParamR:
        if (HPS_IS_TCHAR(_c)) h_state = HPS::kTransferOnParamR;
        else if (!HPS_IS_WHITESPACE(_c)) return HPS::kDead;
        break;
      case HPS::kTransferOnParamR:
        if (HPS_IS_WHITESPACE(_c)) h_state = HPS::kTransferEndParamR;
        else if (_c == ';') h_state = HPS::kTransferNeedParamL;
        else if (_c == ',') h_state = HPS::kTransferNeedValue;
        else if (HPS_IS_NEWLINE(_p)) {
          this->readNBytes(_c == '\n' ? 1 : 2);
          return HPS::kHeaderFieldStart;
        }
        else if(!HPS_IS_TCHAR(_c)) return HPS::kDead;
        break;
      case HPS::kTransferEndParamR:
        if (_c == ';') h_state = HPS::kTransferNeedParamL;
        else if (_c == ',') h_state = HPS::kTransferNeedValue;
        else if (HPS_IS_NEWLINE(_p)) {
          this->readNBytes(_c == '\n' ? 1 : 2);
          return HPS::kHeaderFieldStart;
        }
        else if(!HPS_IS_WHITESPACE(_c)) return HPS::kDead;
        break;
      default:
        return HPS::kDead;
    }
    this->readNBytes(1);
  }
}

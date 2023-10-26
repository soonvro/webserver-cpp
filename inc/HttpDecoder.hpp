#ifndef HTTPDECODER_HPP_
#define HTTPDECODER_HPP_

#include "HttpDecoderEnums.h"
#include "HttpDecoderCallback.h"

class HttpDecoder {
 public:
  HttpDecoder();
  ~HttpDecoder();

  void setCallback(
      t_http_cb      on_message_begin, t_http_data_cb on_url,
      t_http_data_cb on_status,        t_http_data_cb on_header_field,
      t_http_data_cb on_header_value,  t_http_cb      on_headers_complete,
      t_http_data_cb on_body,          t_http_cb      on_message_complete,
      t_http_cb      on_chunk_header,  t_http_cb      on_chunk_complete);
  void setDataSpace(void* data);
  unsigned int execute(const char* buf, const unsigned int len);

  enum HPS::Method       _method;
  unsigned short         _http_major;
  unsigned short         _http_minor;
  bool                   _uses_transfer_encoding;
  unsigned long long     _content_length; /* When on_chunk_header is called,
                                             the current chunk length is stored */
  unsigned char          _flag;
  enum HPS::DecoderErrno _errno;

  void* _data; //request
 private:
  HttpDecoder(const HttpDecoder& rhs);
  HttpDecoder& operator=(const HttpDecoder& rhs);

  bool isStrNCaseInsensOfLhs(const char* lhs_start, const char* lhs_end,
                             const char* rhs, unsigned int rhs_len);
  void readNBytes(unsigned int n);
  bool isNormalUrlChar(const char c);

  enum HPS::DecoderState checkMethod(void);
  enum HPS::DecoderState checkUrl(const char* const& url_start);
  enum HPS::DecoderState checkHttpVer(void);
  enum HPS::DecoderState checkConnection(void);
  enum HPS::DecoderState checkContentLength(void);
  enum HPS::DecoderState checkTransferEncoding(void);

  enum HPS::DecoderState _state;
  const char*            _buf;
  unsigned int           _buf_len;
  const char*            _p; // The current position within the _buf.
  char                   _c; // The current char within the _buf.
  unsigned int           _n_read; // The char bytes that has been read.

  HttpDecoderCallback _cb;
};

#endif

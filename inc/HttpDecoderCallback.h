#ifndef HTTPDECODERCALLBACK_H_
#define HTTPDECODERCALLBACK_H_

class HttpDecoder;

typedef bool (*t_http_data_cb) (HttpDecoder*, const char *at, unsigned int len);
typedef bool (*t_http_cb) (HttpDecoder*);

struct HttpDecoderCallback {
  t_http_cb      on_message_begin;
  t_http_data_cb on_url;
  t_http_data_cb on_status;
  t_http_data_cb on_header_field;
  t_http_data_cb on_header_value;
  t_http_cb      on_headers_complete;
  t_http_data_cb on_body;
  t_http_cb      on_message_complete;
  t_http_cb      on_chunk_header;
  t_http_cb      on_chunk_complete;
};

#endif

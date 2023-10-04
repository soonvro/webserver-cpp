#ifndef HTTPDECODERENUMS_H_
#define HTTPDECODERENUMS_H_

#define HTTP "HTTP"
#define CONNECTION "connection"
#define CONTENT_LENGTH "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define CHUNKED "chunked"
#define KEEP_ALIVE "keep-alive"
#define CLOSE "close"

enum HttpDecoderMethod {
#define GET "GET"
  kGet = 0,
#define POST "POST"
  kPost,
#define DELETE "DELETE"
  kDelete
};

enum HttpDecoderState {
  kStDead,
  kStStart,
  kStMethodStart,
  kStHttpStart,

  s_req_spaces_before_url,
  s_req_schema,
  s_req_schema_slash,
  s_req_schema_slash_slash,
  s_req_server_start,
  s_req_server,
  s_req_server_with_at,
  s_req_path,
  s_req_query_string_start,
  s_req_query_string,
  s_req_fragment_start,
  s_req_fragment,
  s_req_http_major,
  s_req_http_dot,
  s_req_http_minor,
  s_req_http_end,

  s_header_field_start,
  s_header_field,
  s_header_value_discard_ws,
  s_header_value_discard_ws_almost_done,
  s_header_value_discard_lws,
  s_header_value_start,
  s_header_value,
  s_header_value_lws,

  s_header_almost_done,

  s_chunk_size_start,
  s_chunk_size,
  s_chunk_parameters,
  s_chunk_size_almost_done,

  s_headers_almost_done,
  s_headers_done,

  /* Important: 's_headers_done' must be the last 'header' state. All
   * states beyond this must be 'body' states. It is used for overflow
   * checking. See the PARSING_HEADER() macro.
   */

  s_chunk_data,
  s_chunk_data_almost_done,
  s_chunk_data_done,

  s_body_identity,
  s_body_identity_eof,

  s_message_done,
};

enum HttpDecoderHeaderState {
};

#endif

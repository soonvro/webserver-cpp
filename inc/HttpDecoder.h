#ifndef HTTPDECODER_H_
#define HTTPDECODER_H_

#include "HttpDecoderEnums.h"
#include "HttpDecoderCallback.h"

class HttpDecoder {
 public:
  int execute(const char* buf, const unsigned int len);

  unsigned short         _http_major;
  unsigned short         _http_minor;
  enum HttpDecoderMethod _method;

  void* _data; //request
 private:
  enum HttpDecoderState       _state;
  enum HttpDecoderHeaderState _header_state;
  bool         _uses_transfer_encoding;
  unsigned int _nread;
  unsigned int _content_length; /* When on_chunk_header is called,
                                  the current chunk length is stored */
  HttpDecoderCallback _cb;
};

#endif

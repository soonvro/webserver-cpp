#ifndef HTTPDECODERENUMS_H_
#define HTTPDECODERENUMS_H_

#define HTTP "HTTP"
#define HOST "host"
#define CONNECTION "connection"
#define CONTENT_LENGTH "content-length"
#define TRANSFER_ENCODING "transfer-encoding"
#define CHUNKED "chunked"
#define KEEP_ALIVE "keep-alive"
#define CLOSE "close"

#define GET    "GET"
#define HEAD   "HEAD"
#define POST   "POST"
#define DELETE "DELETE"

// HPS(Http ParSer)
namespace HPS {
  enum Method {
    kNotMethod = 0,
    kGET       = 1,
    kHEAD      = 2,
    kPOST      = 3,
    kDELETE    = 4
  };

  enum DecoderFlags {
    kFlagChunked               = 1 << 0,
    kFlagConnectionKeepAlive   = 1 << 1,
    kFlagConnectionClose       = 1 << 2,
    kFlagTrailing              = 1 << 3,
    kFlagSkipbody              = 1 << 4,
    kFlagContentlength         = 1 << 5
  };

  enum DecoderUrlFields {
    kSchema           = 0,
    kHost             = 1,
    kPort             = 2,
    kPath             = 3,
    kQuery            = 4,
  };

  enum DecoderErrno {
    kSuccess,
    kFailure
  };

  enum DecoderState {
    kDead,
    kMethodStart,
    kMethodEnd,
    kUrlStart,
    kUrlSchema,
    kUrlPath,
    kUrlQuery,
    kUrlEnd,
    kHttpVerStart,
    kHttpVerEnd,
    kHeaderFieldStart,
    kHeaderConnection,
    kHeaderContentLength,
    kHeaderTransferEncoding,
    kHeaderFieldEnd,
    kHeaderValueStart,
    kHeaderValueEnd,
    kMessageEnd,
  };

  enum DecoderHeaderState {
    kTransferStart,
    kTransferNeedValue,
    kTransferOnValue,
    kTransferEndValue,
    kTransferNeedParamL,
    kTransferOnParamL,
    kTransferEndParamL,
    kTransferNeedParamR,
    kTransferOnParamR,
    kTransferEndParamR,
  };
}

#endif

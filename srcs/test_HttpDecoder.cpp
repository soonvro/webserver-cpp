#include <iostream>
#include <cstring>
#include "HttpDecoder.hpp"
#include "HttpRequest.hpp"

#define PRINT_DETAIL 0
#define PRINT_REQUEST 1

const char* g_case_ok[] = {
//   "GET / HTTP/1.1\r\n\r\n",
//   "GET / HTTP/1.1\n\n",
//   "GET / HTTP/1.1\n\r\n",
//   "GET / HTTP/1.1\r\n\n",
   "GET / HTTP/1.1\r\nHost: naver.com\n\n",
//   "GET /index.html HTTP/1.1\r\n\n",
//   "GET http://www.naver.com HTTP/1.1\r\n\n",
//   "GET http://www.naver.com:4242 HTTP/1.1\r\n\n",
//   "GET http://www.naver.com/index.html HTTP/1.1\r\n\n",
//   "GET http://www.naver.com:4242/index.html HTTP/1.1\r\n\n",
//   "GET http://192.168.0.0 HTTP/1.1\r\n\n",
//   "GET http://192.168.0.0:4242 HTTP/1.1\r\n\n",
//   "GET http://192.168.0.0/index.html HTTP/1.1\r\n\n",
//   "GET http://192.168.0.0:4242/index.html HTTP/1.1\r\n\n",
//   "GET http://192.168.0.0:4242/index.html/next HTTP/1.1\r\n\n",
//   "GET http://192.168.0.0:4242/index.html/next?name=boo HTTP/1.1\r\n\n",
//   "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nHOst: totoro\n\n",
//   "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nHOst: totoro\n\n",
//   "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: totoro\n\n",
//   "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: ,  , toto, ,,\n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: ,  ,, ,,\nhost: a\n\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4\n\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4, 4 , , \n\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4, 4 , , 4\n\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4, 4 ,\nHost: webserv\n\n",
//   "POST /submit-data HTTP/1.1\nHost: www.example.com\nContent-Type: application/x-www-form-urlencoded\nContent-Length: 23\n\n",
//   "GET / HTTP/1.1\nHost: www.example.com\nConnection: keep-alive\n\n",

//   "GET / HTTP/1.1\nHost: www.example.com\nConTent-Length: 4, 4 ,\nConnection: keep-alive\n\n",
//   "GET / HTTP/1.1\nHost: www.example.com\nConTent-Length: 4, 4 ,\nConnection: keep-alive\nX-header: hahaha\n\n",
};

const char* g_case_failure[] = {
//   "GET / HTTP/1.1\r\n",
//   "GET / HTTP/1.1\r\r\n",
//   "GET / HTTP/1.1\r\n\r",
//   "GET  / HTTP/1.1\r\n\r\n",
//   "GET /  HTTP/1.1\r\n\r\n",
//   "GET / HTTP 1.1\r\n\r\n",
//   "GET / hTTP/1.1\r\n\r\n",
//   "GET /your site/ HTTP/1.1\r\n\n",
//   "GET POST / HTTP/1.1\n\n",
//   "FETCH /index.html HTTP/1.1\n\n",
//   "GET / HTTP/1.1\r\nHost:\n\n",
//   "GET /index.html\nHost: www.example.com\n\n",
//   "GET / HTTP/1.1\r\nConTent-Length:\n\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4, 94 , , 4\n\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4, 4 ,\nHost: webserv\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4, 4 ,\nHost: webserv\nTransfer-encoding: chunked,chunked\n\n",
//   "GET / HTTP/1.1\r\nConTent-Length: 4, 4 ,\nHost: webserv\nTransfer-encoding: s,chunked\n\n",
//   "GET / HTTP/1.1\nHost: www.example.com\nConnection: keep-alive, close\n\n",
//   "GET / HTTP/1.1\nHost: www.example.com\nConnection: keep-alive\nConnection: close\n\n",
  "GET /index.h tml HTTP/1.1\r\n\n",
  "GET https://www.naver.com HTTP/1.1\r\n\n",
  "GET http:/www.naver.com HTTP/1.1\r\n\n",
  "GET http:// HTTP/1.1\r\n\n",
  "GET http://:4242 HTTP/1.1\r\n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: totoro\nHOst: totoro\n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: haha ha\nX-header: hahaha\nHOst: totoro\n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: tot oro\n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: \n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: ,  ,, ,,\n\n",
  "GET http://192.168.0.0:4242/index.html/next?name=boo&age=23 HTTP/1.1\r\nX-header: hahaha\nX-header: hahaha\nHOst: ,  , to to, ,,\n\n",
};

const char* g_methods[] = {"GET", "HEAD", "POST", "DELETE"};

void doTest(const char* test_case) {
  using namespace std;
  HttpRequest hrq;
  HttpDecoder hd;
  hd.setCallback(
      NULL, HttpRequest::sParseUrl,
      NULL, HttpRequest::sSaveHeaderField,
      HttpRequest::sParseHeaderValue, HttpRequest::sSaveRquestData,
      NULL, NULL,
      NULL, NULL);
  hd.setDataSpace(static_cast<void*>(&hrq));

  unsigned int n_read =
    hd.execute(test_case, static_cast<unsigned int>(std::strlen(test_case)));
  if (static_cast<unsigned int>(std::strlen(test_case)) != n_read) {
    cout << "error: n_read is not correct. n_read: " << n_read <<
            ", Faliure Flag: " <<
            ((hd._errno == HPS::kFailure) ? "Failure" : "Success") << endl;
    return;
  }

  cout << "Success" << endl;
  if (PRINT_DETAIL) {
    cout << " method: " << g_methods[hd._method - 1] <<
            ", HTTP/" << hd._http_major << '.' << hd._http_minor <<
            ", use trasfer? " << (hd._uses_transfer_encoding ? "yes" : "no") <<
            ", content-length: " << hd._content_length << endl;
    cout << " flag: " << endl;
    cout << "  FlagChunked            : " <<
            ((hd._flag & HPS::kFlagChunked) ? 1 : 0) << endl;
    cout << "  FlagConnectionKeepAlive: " <<
            ((hd._flag & HPS::kFlagConnectionKeepAlive) ? 1 : 0) << endl;
    cout << "  FlagConnectionClose    : " <<
            ((hd._flag & HPS::kFlagConnectionClose) ? 1 : 0) << endl;
    cout << "  FlagTrailing           : " <<
            ((hd._flag & HPS::kFlagTrailing) ? 1 : 0) << endl;
    cout << "  FlagSkipbody           : " <<
            ((hd._flag & HPS::kFlagSkipbody) ? 1 : 0) << endl;
    cout << "  FlagContentlength      : " <<
            ((hd._flag & HPS::kFlagContentlength) ? 1 : 0) << endl;
  }
  if (PRINT_REQUEST) {
    cout << "== Request ==" << endl;
    cout << hrq.getHost() <<
            hrq.getLocation() <<
            '?' << hrq.getQueries() << endl;
    std::map<std::string, std::string> headers = hrq.getHeaders();
    for (std::map<std::string, std::string>::iterator iter = headers.begin();
         iter != headers.end(); iter++) {
      cout << iter->first << ": " << iter-> second << endl;
    }
  }
}

void test_HttpDecoder(void) {
  using namespace std;

  for (int i = 0; i < static_cast<int>(sizeof(g_case_ok)/sizeof(g_case_ok[0])); i++) {
    doTest(g_case_ok[i]);
  }
  cout << endl;
  for (int i = 0; i < static_cast<int>(sizeof(g_case_failure)/sizeof(g_case_failure[0])); i++)
    doTest(g_case_failure[i]);
}

// int main() {
//   test_HttpDecoder();
// }

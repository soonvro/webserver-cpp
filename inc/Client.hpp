#ifndef CLIENT_HPP_
#define CLIENT_HPP_

#include <queue>
#include <map>

#include "HttpRequest.hpp"
#include "HttpResponse.hpp"

#define CLIENT_BUF_SIZE 134217728

class Client {
  private:
    std::vector<char>                 _buf;
    size_t                            _read_idx;

    std::queue<HttpRequest>           _reqs;
    std::queue<HttpResponse>          _ress;

    bool                              _has_eof;

    int                               _client_fd;
    int                               _port;
    time_t                            _last_request_time;
    time_t                            _timeout_interval;
    bool                              _is_time_out;
    int                               _time_out_message_idx;

    std::map<int, HttpResponse*>       _http_responses_by_fd;
    std::map<int, HttpResponse*>       _http_responses_by_pid;

  public:
    Client();
    Client(int client_fd, int port, time_t last_request_time, time_t timeout_interval);

    Client& operator=(const Client& other);

    const std::vector<char>&            getBuf(void) const;
    const size_t&                       getReadIdx(void) const;
    const std::queue<HttpRequest>&      getReqs(void) const;
    std::queue<HttpResponse>&           getRess(void);
    const bool&                         getEof(void) const;
    const int&                          getClientFd() const;
    const int&                          getPort(void) const;
    const time_t&                       getLastRequestTime() const;
    const time_t&                       getTimeoutInterval() const;
    const bool&                         getIsTimeOut(void) const;
    const int&                          getTimeOutMessageIdx(void) const;

    std::vector<char>::const_iterator   getReadIter(void);
    std::vector<char>::const_iterator   getEndIter(void);

    HttpResponse&                       getResponseByCgiFd(int fd);
    HttpResponse&                       getResponseByPid(int pid);

    HttpResponse&                       backRess(void);

    HttpRequest&                        backRequest(void);

    void                                clearBuf(void);

    void                                addBuf(const char* buf, size_t size);
    void                                addReadIdx(size_t idx);
    Client&                             addReqs(void);
    Client&                             addRess(const HttpRequest& req, const RouteRule* route_rule);


    void                                eraseBuf(void);
    void                                popReqs(void);
    void                                popRess(void);
    void                                setEof(bool has_eof);

    int                                 headerEndIdx(const size_t& start);
    const std::vector<char>             subBuf(const size_t start, const size_t end);

    void                                setLastRequestTime(const time_t& last_request_time);
    void                                setTimeoutInterval(const time_t& timeout_interval);
    void                                setIsTimeOut(const bool& is_time_out);
    void                                setTimeOutMessageIdx(const int& idx);

    void                                addResponseByFd(int fd, HttpResponse* res);
    void                                addResponseByPid(int fd, HttpResponse* res);

    bool                                operator<(const Client& other) const;
};

#endif

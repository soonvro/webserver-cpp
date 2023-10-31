#include "CgiHandler.hpp"
#include <unistd.h>


void  CgiHandler::addBuf(const char* buf, size_t size){ _buf.insert(_buf.end(), buf, buf + size); }

void  CgiHandler::closeReadPipe(void){ close(_pipe_fd[0]); }

const int&                CgiHandler::getClientFd(void) const{ return _client_fd; }
const std::vector<char>&  CgiHandler::getBuf(void) const{ return _buf; }
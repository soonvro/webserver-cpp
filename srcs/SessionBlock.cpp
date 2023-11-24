#include "SessionBlock.hpp"

time_t getTime(void) {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return tv.tv_sec;
}

char  SessionBlock::generateRandomChar(void) {
  const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
  int index = rand() % (sizeof(charset) - 1);
  return charset[index];
}
void  SessionBlock::generateSessionId(int size) {
  srand((unsigned int)time(NULL));
  for (int i = 0;i < size; ++i) {
    _id += generateRandomChar();
  }
}

SessionBlock::SessionBlock() : _id(), _value(), _expires(0) {
  // random session id
  generateSessionId(5);
  _expires = getTime();
}

const std::string&  SessionBlock::getId(void) const { return _id; }
const std::string&  SessionBlock::getValue(void) const { return _value; }
const long&         SessionBlock::getExpires(void) const { return _expires; }

void                SessionBlock::setId(const std::string& id) { _id = id; }

void                SessionBlock::setValue(const std::string& value) { _value = value; }
void                SessionBlock::renewExp(void) { _expires = getTime(); }
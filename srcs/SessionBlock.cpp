#include "SessionBlock.hpp"
#include <iostream>
SessionBlock::SessionBlock() : _id(), _value(), _expires(0) {
  // random session id
  _expires = 843541684635;
  _id = "sdfsdfsdfsdfds_dsfsd";
}

const std::string&  SessionBlock::getId(void) const { return _id; }
const std::string&  SessionBlock::getValue(void) const { return _value; }
const long&         SessionBlock::getExpires(void) const { return _expires; }

void                SessionBlock::setValue(std::string& value) { _value = value; }
void                SessionBlock::renewExp(void) { _expires = static_cast<long>(clock()) / CLOCKS_PER_SEC; }
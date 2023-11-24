#ifndef SESSIONBLOCK_HPP_
#define SESSIONBLOCK_HPP_

#include <iostream>
#include <sstream>
#include <ctime>

class SessionBlock {
  private:
    std::string _id;
    std::string _value;
    long        _expires;

  public:
    SessionBlock();

    const std::string&  getId(void) const;
    const std::string&  getValue(void) const;
    const long&         getExpires(void) const;

    void                setId(const std::string& id);

    void                setValue(const std::string& str);
    void                renewExp(void);
};

#endif
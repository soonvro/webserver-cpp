#ifndef SESSIONBLOCK_HPP_
#define SESSIONBLOCK_HPP_

#include <iostream>
#include <sstream>
#include <sys/time.h>
#include <ctime>
#include <cstdlib>

class SessionBlock {
  private:
    std::string _id;
    std::string _value;
    time_t      _expires;

    char  generateRandomChar(void);
    void  generateSessionId(int size);

  public:
    SessionBlock();

    const std::string&  getId(void) const;
    const std::string&  getValue(void) const;
    const time_t&       getExpires(void) const;

    void                setId(const std::string& id);

    void                setValue(const std::string& str);
    void                renewExp(void);
};

#endif
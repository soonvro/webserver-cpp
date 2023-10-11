#ifndef ENCODER_HPP_
#define ENCODER_HPP_

#include "HttpResponse.hpp"
#include <string>

class Encoder{
  public:
    static std::string execute(const HttpResponse& res);
};

#endif
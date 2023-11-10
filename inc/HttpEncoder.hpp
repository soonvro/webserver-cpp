#ifndef ENCODER_HPP_
#define ENCODER_HPP_

#include <string>

#include "HttpResponse.hpp"

class HttpEncoder{
  public:
    static std::string execute(const HttpResponse& res);
};

#endif
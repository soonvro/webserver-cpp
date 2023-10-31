#include "CgiResponse.hpp"
#include <sstream>

CgiResponse::CgiResponse(std::string& s){
  std::stringstream ss(s);
  std::string line;


  getline(ss, line);
  std::stringstream ss2(line);
  std::string word;
  ss2 >> word;
  if (word == "Content-Type:"){
    ss2 >> _content_type;
  }else if (word == "Location:"){
    ss2 >> _location;
  }else{
    //cgi error
  }
}
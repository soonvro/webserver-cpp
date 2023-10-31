#include "CgiResponse.hpp"
#include <sstream>

CgiResponse::CgiResponse(std::string& s){
  std::stringstream ss(s);
  std::string line;


  getline(ss, line);
  std::stringstream ss2(line);
  std::string word;
  ss2 >> word;

}

const std::string&                        CgiResponse::getContentType(void) const { 
  if (_headers.find("Content-Type") != _headers.end())
    return _headers.find("Content-Type")->second;
  return "";
}

const std::string&                        CgiResponse::getLocation(void) const { 
  if (_headers.find("Location") != _headers.end())
    return _headers.find("Location")->second;
  return "";
}

const std::vector<char>&                  CgiResponse::getBody(void) const { return _body; }
const CgiType&                            CgiResponse::getType(void) const { return _type; }
const unsigned short&                     CgiResponse::getStatus(void) const { return _status; }
const std::map<std::string, std::string>& CgiResponse::getHeaders(void) const { return _headers; }


#include "CgiResponse.hpp"
#include <sstream>

CgiResponse::CgiResponse(std::string& s){
  std::stringstream ss(s);
  std::string line;

  getline(ss, line);
  std::stringstream ss_one_line(line);
  std::string key;
  std::string value;
  ss_one_line >> key >> value;
  if (key == "Content-Type:") {
    _type = kDocument;
    _headers["Content-Type"] = value;
  } else if (key == "Location:") {
    _headers["Location"] = value;
    if (key[0] == '/') {
      _type = kLocalRedir;
    }
    else {
      _type = kClientRedir;
    }
  } else {
    throw std::runtime_error("invalid cgi response");
  }
  //get other header
  while (1){
    getline(ss, line);
    if (line == "")
      break;
    ss_one_line.str(line);
    ss_one_line >> key >> value;
    key.pop_back();
    _headers[key] = value;
    if (ss.eof())
      return ;
  }
  //get body
  ss >> line;
  _body.assign(line.begin(), line.end());
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


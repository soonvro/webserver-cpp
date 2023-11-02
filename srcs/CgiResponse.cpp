#include "CgiResponse.hpp"
#include <sstream>

CgiResponse::CgiResponse(std::string& s){
  std::string line;
  std::string key;
  std::string value;
  std::stringstream ss(s);
  std::stringstream ss_one_line;

  while (1){
    std::getline(ss, line);
    if (line == "\r" || line == "")
      break;
    ss_one_line.str(line);
    ss_one_line >> key >> value;
    key.pop_back();
    value.pop_back();
    _headers[key] = value;
    if (ss.eof())
      break ;
  }
  if (_headers.find("Location") != _headers.end()){
    value = _headers["Location"];
    if (value[0] == '/'){
      _status = 300;
      _type = kLocalRedir;
    }else{
      _status = 302;
      _type = kClientRedir;
    }
  } else if (_headers.find("Content-type") != _headers.end() || _headers.find("Content-Type") != _headers.end()) {
      _status = 200;
      _type = kDocument;
  } else {
      _status = 404;
      _type = kError;
  }
  //get body
  line = ss.str().substr(ss.tellg());

  _body.assign(line.begin(), line.end());
  if (_type == kClientRedir && _body.size() > 0){
    _type = kClientRedirDoc;
  }
}

const std::vector<char>&                  CgiResponse::getBody(void) const { return _body; }
const CgiType&                            CgiResponse::getType(void) const { return _type; }
const unsigned short&                     CgiResponse::getStatus(void) const { return _status; }
const std::map<std::string, std::string>& CgiResponse::getHeaders(void) const { return _headers; }


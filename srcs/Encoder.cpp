#include "Encoder.hpp"
#include "HttpResponse.hpp"
#include <iterator>


std::string Encoder::execute(const HttpResponse& res){
  std::stringstream ss;
  std::string encoded_response = "HTTP/";
  ss << res.getHttpMajor() << "." << res.getHttpMinor();
  encoded_response += ss.str();
  encoded_response += " ";
  ss << res.getStatus();
  encoded_response += ss.str();
  encoded_response += " ";
  encoded_response += res.getStatusMessage();
  encoded_response += "\r\n";
  
  const std::map<std::string, std::string>& headers = res.getHeader();
  std::map<std::string, std::string>::const_iterator it = headers.begin();
  while (it != headers.end()){
    encoded_response += it->first;
    encoded_response += ": ";
    encoded_response += it->second;
    encoded_response += "\r\n";
    it++;
  }
  encoded_response += "\r\n";
  return encoded_response;
}
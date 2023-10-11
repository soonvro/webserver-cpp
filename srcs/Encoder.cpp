#include "Encoder.hpp"
#include "HttpResponse.hpp"
#include <iterator>

std::string Encoder::execute(const HttpResponse& res){
  std::string encoded_response = "HTTP/1.1";
  encoded_response += " ";
  encoded_response += std::to_string(res.getStatus());
  encoded_response += " ";
  encoded_response += res.getStatusMessage();
  encoded_response += "\r\n";
  
  std::map<std::string, std::string>::iterator it = res.getHeader();
  while (it != res.end()){
    encoded_response += it->first;
    encoded_response += ": ";
    encoded_response += it->second;
    encoded_response += "\r\n";
    it++;
  }
  encoded_response += "\r\n";
  return encoded_response;
}
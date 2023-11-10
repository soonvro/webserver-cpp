#ifndef CGI_RESPONSE_HPP_
#define CGI_RESPONSE_HPP_

#include <string>
#include <map>
#include <vector>

enum CgiType{
  kDocument,
  kLocalRedir,
  kClientRedir,
  kClientRedirDoc,
  kError
};

class CgiResponse{
  public:
    CgiResponse(std::string& s);

    const std::vector<char>&                        getBody(void) const;
    const CgiType&                                  getType(void) const;
    const unsigned short&                           getStatus(void) const;
    const std::map<std::string, std::string>&       getHeaders(void) const;

  private:
    std::map<std::string, std::string>              _headers;
    std::vector<char>                               _body;
    CgiType                                         _type;
    unsigned short                                  _status;
};

#endif
#ifndef CGI_RESPONSE_HPP_
#define CGI_RESPONSE_HPP_

#include <string>
#include <map>

enum CgiType{
  kDocument,
  kLocalRedir,
  kClientRedir,
  kClientRedirDoc
};

class CgiResponse{
  public:
    CgiResponse(std::string& s);

    const std::string&                        getContentType(void) const;
    const std::string&                        getLocation(void) const;
    const std::string&                        getBody(void) const;
    const CgiType&                          getType(void) const;
    const std::string&                        getStatusCode(void) const;
    const std::map<std::string, std::string>& getHeaders(void) const;

  private:
    std::map<std::string, std::string> _headers;
    std::string _body;
    CgiType _type;
    std::string _status_code;
};

#endif

  //1. document-response = Content-Type [ Status ] *other-field NL response-body
  //헤더 그대로 붙여서 보내기
  //2. local-redir-response = local-Location NL
  //3. lient-redir-response = client-Location *extension-field NL
  //302 found 
  //4. lient-redirdoc-response = client-Location Status Content-Type *other-field NL response-body
  //302 found 
  //status: digit message ;
  //location header 
  //entity 추가
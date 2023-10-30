#ifndef CGI_RESPONSE_HPP_
#define CGI_RESPONSE_HPP_

#include <string>
#include <map>

enum CgiStatus{
  kDocumentResponse,
  kLocalRedirResponse,
  kClientRedirResponse,
  kClientRedirDocResponse
};

class CgiResponse{
  public:
    CgiResponse(std::string& s);

  private:
    std::map<std::string, std::string> _headers;
    std::string _body;
    CgiStatus _status;
    std::string _content_type;
    std::string _location;
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
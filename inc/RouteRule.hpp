#ifndef ROUTERULE_H
#define ROUTERULE_H

#include <map>
#include <string>

#define DEFAULT_ROOT "html"

class RouteRule {
 private:
  std::string                   _route;
  std::string                   _location;
  int                           _accepted_methods;  
// 비트 자릿수로 allow 확인 get 1 head 2 post 3 delete 4

  std::pair<int, std::string>    _redirection;
  bool                          _autoIndex;
  std::string                   _index_page;
  std::map<int, std::string>    _error_pages;
  bool                          _is_client_body_size_set;
  size_t                        _max_client_body_size;
  bool                          _isCgi;
  std::string                   _cgiPath;

 public:
  RouteRule();

  bool hasRedirection(int code) const;
  void addRedirection(int code, const std::string& url);

  bool hasErrorPage(int code) const;
  void addErrorPage(int code, const std::string& url);

  const std::string                   getRoot() const;


  const std::string&                  getRoute() const;
  const std::string&                  getLocation() const;
  int                                 getAcceptedMethods() const;
  const std::pair<int, std::string>&  getRedirection() const;
  bool                                getAutoIndex() const;
  const std::string&                  getIndexPage() const;
  const std::string&                  getErrorPage(int code) const;
  bool                                getIsClientBodySizeSet() const;
  size_t                              getMaxClientBodySize() const;
  bool                                getIsCgi() const;
  const std::string&                  getCgiPath() const;

  void                                setRoute(const std::string& route);
  void                                setLocation(const std::string& location);
  void                                setAcceptedMethods(int methods);
  void                                setRedirection(const std::pair<int, std::string>& redirection);
  void                                setAutoIndex(bool enable);
  void                                setIndexPage(const std::string& index);
  void                                setIsClientBodySizeSet(bool isSet);
  void                                setMaxClientBodySize(size_t maxSize);
  void                                setIsCgi(bool isCgi);
  void                                setCgiPath(const std::string& cgiPath);
};

#endif

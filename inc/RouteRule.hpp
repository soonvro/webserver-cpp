#ifndef ROUTERULE_H
#define ROUTERULE_H

#include <map>
#include <string>

class RouteRule {
 private:
  std::string _route;
  std::string _location;
  int _accepted_methods;  // 비트 자릿수로 allow 확인 get 1 head 2 post 3 delete
                          // 4
  std::map<int, std::string> _redirection;
  bool _autoIndex;
  std::string _index_page;
  std::map<int, std::string> _error_pages;
  bool _isClientBodySizeSet;
  size_t _max_client_body_size;
  bool _isCgi;
  std::string _cgiPath;

 public:
  RouteRule();

  const std::string& getRoute() const;
  void setRoute(const std::string& route);

  const std::string& getLocation() const;
  void setLocation(const std::string& location);

  int getAcceptedMethods() const;
  void setAcceptedMethods(int methods);

  const std::map<int, std::string>& getRedirection() const;
  void setRedirection(const std::map<int, std::string>& redirection);

  bool hasRedirection(int code) const;
  void addRedirection(int code, const std::string& url);

  bool getAutoIndex() const;
  void setAutoIndex(bool enable);

  const std::string& getIndexPage() const;
  void setIndexPage(const std::string& index);

  bool hasErrorPage(int code) const;
  void addErrorPage(int code, const std::string& url);

  bool getIsClientBodySizeSet() const;
  void setIsClientBodySizeSet(bool isSet);

  size_t getMaxClientBodySize() const;
  void setMaxClientBodySize(size_t maxSize);

  void setIsCgi(bool isCgi);
  bool getIsCgi() const;

  void setCgiPath(const std::string& cgiPath);
  const std::string& getCgiPath() const;
};

#endif  // ROUTERULE_H

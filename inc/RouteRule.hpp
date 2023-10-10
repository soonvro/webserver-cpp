#ifndef ROUTERULE_H
#define ROUTERULE_H

#include <string>
#include <utility>

class RouteRule {
private:
    std::string _route;
    std::string _location;
    int _accepted_methods;
    std::pair<int, std::string> _redirection;
    bool _directory_listing;
    std::string _default_response_for_directory;

public:
    const std::string& getRoute() const;
    void setRoute(const std::string& route);

    const std::string& getLocation() const;
    void setLocation(const std::string& location);

    int getAcceptedMethods() const;
    void setAcceptedMethods(int methods);

    const std::pair<int, std::string>& getRedirection() const;
    void setRedirection(const std::pair<int, std::string>& redirection);

    bool isDirectoryListingEnabled() const;
    void setDirectoryListing(bool enable);

    const std::string& getDefaultResponseForDirectory() const;
    void setDefaultResponseForDirectory(const std::string& response);
};

#endif // ROUTERULE_H

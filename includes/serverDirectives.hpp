#pragma once
#include <string>
#include <vector>
#include "  locationDirectives.hpp"
class serverDirectives {
    private:
        std::string _root;
        std::string _index;
        std::string _cgiExtension;
        std::string _cgiPath;
        std::string _autoIndex;
        std::string _allowedMethods;

        std::string _listen;
        std::string _serverName;
        std::vector<locationDirectives> _locations;
        std::string client_max_body_size;
        std::vector<std::string> _errorPages;



    public:
        serverDirectives();
        ~serverDirectives();
        void setListen(const std::string& listen);
        void setServerName(const std::string& serverName);
        void setRoot(const std::string& root);
        void setIndex(const std::string& index);
        void setCgiExtension(const std::string& cgiExtension);
        void setCgiPath(const std::string& cgiPath);
        void setAutoIndex(const std::string& autoIndex);
        void setAllowedMethods(const std::string& allowedMethods);
        void setClientMaxBodySize(const std::string& client_max_body_size);
        void addLocation(const locationDirectives& location);
        void addErrorPage(const std::string& errorPage);
        const std::string& getListen() const;
        const std::string& getServerName() const;
        const std::string& getRoot() const;
        const std::string& getIndex() const;
        const std::vector<std::string>& getErrorPages() const;
        const std::string& getCgiExtension() const;
        const std::string& getCgiPath() const;
        const std::string& getAutoIndex() const;
        const std::string& getAllowedMethods() const;
        const std::string& getClientMaxBodySize() const;
        const std::vector<locationDirectives>& getLocations() const;
};
#pragma once
#include <string>
#include <vector>
#include <stdexcept>

class LocationConfig {
    // root ./webroot;
    private:
        std::string                         _path;          
        std::string                         _root;          
        std::string                         _index;         
        bool                                _autoIndex;     
        std::vector<std::string>            _allowedMethods;
        std::string                         _uploadDir;
        std::string                         _cgiExtension;
        std::string                         _cgiPath;
        std::pair<int , std::string>         _return;


    public:
        LocationConfig();
        ~LocationConfig();
        
        void setPath(const std::string& path);
        void setRoot(const std::string& root);
        void setIndex(const std::string& index);
        void setAutoIndex(bool autoIndex);
        void setAllowedMethods(const std::vector<std::string>& methods);
        void setUploadDir(const std::string& uploadDir);
        void setCgiExtension(const std::string& cgiExtension);
        void setCgiPath(const std::string& cgiPath);
        void setReturn(int statusCode, const std::string& url);

        const std::string& getPath() const;
        const std::string& getRoot() const;
        const std::string& getIndex() const;
        bool getAutoIndex() const;
        const std::vector<std::string>& getAllowedMethods() const;
        const std::string& getUploadDir() const;
        const std::string& getCgiExtension() const;
        const std::string& getCgiPath() const;
        const std::pair<int, std::string>& getReturn() const;

    private:
        void validateAllowedMethods(const std::vector<std::string>& methods);
        void validateAllowedMethod(const std::string& method);
        void validateCgiExtension(const std::string& extension);
        void validateCgiPath(const std::string& path);
        void validateUploadDir(const std::string& uploadDir);
        void validateRoot(const std::string& root);
        void validateIndex(const std::string& index);
        void validatePath(const std::string& path);
        void validateAutoIndex(bool autoIndex);
        void validateReturn(int statusCode, const std::string& url);
};
// /website/mabuyahy
// #location /uploads {
//         #    allowed_methods GET HEAD POST DELETE;
//         #    root ./webroot;
//         #     client_max_body_size 200000;
//         #}
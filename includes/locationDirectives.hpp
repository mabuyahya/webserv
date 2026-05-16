#pragma once
#include <string>

class locationDirectives {
    private:
        std::string _path;
        std::string _root;
        std::string _index;
        std::string _autoIndex;
        std::string _allowedMethods;
        std::string _uploadDir;
        std::string _cgiExtension;
        std::string _cgiPath;

    public:
        locationDirectives();
        ~locationDirectives();
        void setPath(const std::string& path);
        void setRoot(const std::string& root);
        void setIndex(const std::string& index);
        void setAutoIndex(const std::string& autoIndex);
        void setAllowedMethods(const std::string& allowedMethods);
        void setUploadDir(const std::string& uploadDir);
        void setCgiExtension(const std::string& cgiExtension);
        void setCgiPath(const std::string& cgiPath);
        const std::string& getPath() const;
        const std::string& getRoot() const;
        const std::string& getIndex() const;
        const std::string& getAutoIndex() const;
        const std::string& getAllowedMethods() const;
        const std::string& getUploadDir() const;
        const std::string& getCgiExtension() const;
        const std::string& getCgiPath() const;

};
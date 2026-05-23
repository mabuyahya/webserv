#include "../../includes/config/ConfigParser.hpp"
#include "../../includes/config/serverDirectives.hpp"
#include "../../includes/config/locationDirectives.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>
#include <cstdlib>

ConfigParser::ConfigParser(const std::string& path) : _configFile(path) {}

std::vector<ServerConfig> ConfigParser::getServers() const {
    return _servers;
}

static int is_delimiter(char c, const std::string& delimiters)
{
    size_t i;

    i = 0;
    while (i < delimiters.length())
    {
        if (c == delimiters[i])
            return (1);
        i++;
    }
    return (0);
}

static std::vector<std::string> split(const std::string& str, const std::string& delimiters)
{
    std::vector<std::string> result;
    size_t start = 0;
    size_t i = 0;

    while (i < str.length())
    {
        if (is_delimiter(str[i], delimiters))
        {
            if (i > start)
                result.push_back(str.substr(start, i - start));
            start = i + 1;
        }
        i++;
    }

    if (start < str.length())
        result.push_back(str.substr(start));

    return result;
}

static bool starts_with(const std::string& str, const std::string& prefix)
{
    return str.compare(0, prefix.length(), prefix) == 0;
}

static bool ends_with(const std::string& str, const std::string& suffix)
{
    if (str.length() < suffix.length())
        return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

static int my_stoi(const std::string& str) {
    return atoi(str.c_str());
}

static size_t my_stoul(const std::string& str) {
    return strtoul(str.c_str(), NULL, 10);
}

static void checkIfAllMondatoryLocationDirectivesSet(const LocationConfig& locConfig) {
    if (locConfig.getPath().empty()) {
        throw std::runtime_error("Path directive is required in location block");
    }
    if (locConfig.getRoot().empty()) {
        throw std::runtime_error("Root directive is required in location block :" + locConfig.getPath());
    }
    if (locConfig.getIndex().empty()) {
        throw std::runtime_error("Index directive is required in location block :" + locConfig.getPath());
    }
}

static bool hasAllowedMethod(const LocationConfig& locConfig, const std::string& method) {
    const std::vector<std::string>& allowedMethods = locConfig.getAllowedMethods();

    for (size_t i = 0; i < allowedMethods.size(); ++i) {
        if (allowedMethods[i] == method)
            return true;
    }
    return false;
}

static void checkPostLocationHasBodyHandler(const LocationConfig& locConfig) {
    bool hasCgi = !locConfig.getCgiExtension().empty() && !locConfig.getCgiPath().empty();
    bool hasUploadDir = !locConfig.getUploadDir().empty();

    if (hasAllowedMethod(locConfig, "POST") && !hasCgi && !hasUploadDir) {
        throw std::runtime_error("Location block '" + locConfig.getPath() + "' allows POST but has no cgi or upload_dir directive");
    }
}

ServerConfig ConfigParser::parse_server_directives(const serverDirectives& s_directives) {
    ServerConfig config;

    // Validate listen is present
    if (s_directives.getListen().empty()) {
        throw std::runtime_error("Listen directive is required in server block");
    }

    // Parse listen directive
    std::vector<std::string> listenParts = split(s_directives.getListen(), ":");
    if (listenParts.size() == 0 || listenParts.size() > 2) {
        throw std::runtime_error("Invalid listen directive: " + s_directives.getListen());
    }
    if (listenParts.size() == 1) {
        config.setPort(my_stoi(listenParts[0]));
    }
    else {
        config.setHost(listenParts[0]);
        config.setPort(my_stoi(listenParts[1]));
    }

    // Parse server_name directive
    if (!s_directives.getServerName().empty()) {
        config.setServerName(s_directives.getServerName());
    }

    // Parse client_max_body_size directive
    if (!s_directives.getClientMaxBodySize().empty()) {
        config.setClientMaxBodySize(my_stoul(s_directives.getClientMaxBodySize()));
    }

    // Parse error_page directive
    for (size_t i = 0; i < s_directives.getErrorPages().size(); ++i) {
        std::vector<std::string> errorParts = split(s_directives.getErrorPages()[i], " ");
        if (errorParts.size() != 2) {
            throw std::runtime_error("Invalid error_page directive: " + s_directives.getErrorPages()[i]);
        }
        int statusCode = my_stoi(errorParts[0]);
        config.addErrorPage(statusCode, errorParts[1]);
    }

    // Parse location blocks - inherit inheritable directives
    for (size_t i = 0; i < s_directives.getLocations().size(); ++i) {
        const locationDirectives& locDir = s_directives.getLocations()[i];
        LocationConfig locConfig;
        locConfig.setPath(locDir.getPath());

        // Inherit root from server if not set in location
        if (locDir.getRoot().empty()) {
            if (!s_directives.getRoot().empty())
                locConfig.setRoot(s_directives.getRoot());
        } else {
            locConfig.setRoot(locDir.getRoot());
        }

        // Inherit index from server if not set in location
        if (locDir.getIndex().empty()) {
            if (!s_directives.getIndex().empty())
                locConfig.setIndex(s_directives.getIndex());
        } else {
            locConfig.setIndex(locDir.getIndex());
        }

        // Inherit autoindex from server if not set in location
        bool autoIndex = false;
        if (locDir.getAutoIndex().empty()) {
            autoIndex = (s_directives.getAutoIndex() == "on");
        } else {
            autoIndex = (locDir.getAutoIndex() == "on");
        }
        locConfig.setAutoIndex(autoIndex);

        // Inherit allowedMethods from server if not set in location
        std::vector<std::string> allowedMethods;
        if (locDir.getAllowedMethods().empty()) {
            allowedMethods = split(s_directives.getAllowedMethods(), " ");
        } else {
            allowedMethods = split(locDir.getAllowedMethods(), " ");
        }
        locConfig.setAllowedMethods(allowedMethods);

        // Location-only directives
        if (!locDir.getUploadDir().empty()) {
            
            locConfig.setUploadDir(locDir.getUploadDir());
            locConfig._hasUploadDir = true;
        }
        if (!locDir.getReturn().empty()) {
            int statusCode;
            std::string url;
            std::vector<std::string> returnParts = split(locDir.getReturn(), " ");
            if (returnParts.size() != 2) {
                throw std::runtime_error("Invalid return directive in location block: " + locDir.getReturn());
            }
            statusCode = my_stoi(returnParts[0]);
            url = returnParts[1];
            locConfig.setReturn(statusCode, url);
        }
        if (!locDir.getCgiExtension().empty() && !locDir.getCgiPath().empty()) {
            locConfig.setCgiExtension(locDir.getCgiExtension());
            locConfig.setCgiPath(locDir.getCgiPath());
        }
        checkIfAllMondatoryLocationDirectivesSet(locConfig);
        checkPostLocationHasBodyHandler(locConfig);
        config.addLocation(locConfig);
    }
    return config;
}



void ConfigParser::parse() {
    std::ifstream file(_configFile.c_str());
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + _configFile);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = split(line, " \t");
        
        // Skip empty lines and comments
        if (tokens.empty())
            continue;
        if (starts_with(tokens[0], "#"))
            continue;

        if (tokens.size() >= 2 && tokens[0] == "server" && tokens[1] == "{") {
            serverDirectives s_directives;
            std::string blockLine;
            
            while (std::getline(file, blockLine)) {
                std::vector<std::string> blockTokens = split(blockLine, " \t");
                
                if (blockTokens.empty())
                    continue;
                if (starts_with(blockTokens[0], "#"))
                    continue;
                if (blockLine == "}") {
                    break;
                }

                // Remove trailing semicolon
                if (!blockTokens.empty()) {
                    std::string lastToken = blockTokens.back();
                    if (lastToken == ";") {
                        blockTokens.pop_back();
                    } else if (ends_with(lastToken, ";")) {
                        blockTokens.back() = lastToken.substr(0, lastToken.length() - 1);
                    } else if (blockTokens[0] != "location") {
                        throw std::runtime_error("Missing semicolon at the end of directive: " + blockLine);
                    }
                }

                if (blockTokens.size() == 2 && blockTokens[0] == "listen") {
                    s_directives.setListen(blockTokens[1]);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "server_name") {
                    s_directives.setServerName(blockTokens[1]);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "root") {
                    s_directives.setRoot(blockTokens[1]);
                } else if (blockTokens.size() >= 2 && blockTokens[0] == "index") {
                    std::string indexValue;
                    for (size_t i = 1; i < blockTokens.size(); ++i) {
                        if (i > 1)
                            indexValue += " ";
                        indexValue += blockTokens[i];
                    }
                    s_directives.setIndex(indexValue);
                } else if (blockTokens.size() >= 2 && blockTokens[0] == "error_page") {
                    if (blockTokens.size() != 3) {
                        throw std::runtime_error("Invalid error_page directive: " + blockLine);
                    }
                    std::string errorPage = blockTokens[1] + " " + blockTokens[2];
                    s_directives.addErrorPage(errorPage);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "autoindex") {
                    if (blockTokens[1] != "on" && blockTokens[1] != "off") {
                        throw std::runtime_error("Invalid autoindex value: " + blockTokens[1]);
                    }
                    s_directives.setAutoIndex(blockTokens[1]);
                } else if (blockTokens.size() > 1 && blockTokens[0] == "allowed_methods") {
                    if (blockTokens.size() > 4 || blockTokens.size() < 2) {
                        throw std::runtime_error("Invalid allowed_methods directive: " + blockLine);
                    }
                    std::string allowedMethods;
                    for (size_t i = 1; i < blockTokens.size(); ++i) {
                        allowedMethods += blockTokens[i] + " ";
                    }
                    if (!allowedMethods.empty()) {
                        allowedMethods = allowedMethods.substr(0, allowedMethods.length() - 1);
                    }
                    s_directives.setAllowedMethods(allowedMethods);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "client_max_body_size") {
                    s_directives.setClientMaxBodySize(blockTokens[1]);
                } else if (blockTokens.size() >= 2 && blockTokens[0] == "location") {
                    if (blockTokens.size() < 3 || blockTokens[2] != "{") {
                        throw std::runtime_error("Invalid location block syntax");
                    }
                    locationDirectives loc_directives;
                    loc_directives.setPath(blockTokens[1]);
                    std::string locLine;
                    
                    while (std::getline(file, locLine)) {
                        std::vector<std::string> locTokens = split(locLine, " \t");
                        
                            if (locTokens.empty()) {
                                continue;
                            }
                            if (starts_with(locTokens[0], "#"))
                            continue;
                            if (locTokens.size() == 1 && locTokens[0] == "}") {
                            break;
                        }

                        // Remove trailing semicolon
                        if (!locTokens.empty()) {
                            std::string lastToken = locTokens.back();
                            if (lastToken == ";") {
                                locTokens.pop_back();
                            } else if (ends_with(lastToken, ";")) {
                                locTokens.back() = lastToken.substr(0, lastToken.length() - 1);
                            } else {
                                throw std::runtime_error("Missing semicolon at the end of directive in location: " + locLine);
                            }
                        }

                        if (locTokens.size() == 2 && locTokens[0] == "root") {
                            loc_directives.setRoot(locTokens[1]);
                        } else if (locTokens.size() >= 2 && locTokens[0] == "index") {
                            std::string indexValue;
                            for (size_t i = 1; i < locTokens.size(); ++i) {
                                if (i > 1)
                                    indexValue += " ";
                                indexValue += locTokens[i];
                            }
                            loc_directives.setIndex(indexValue);
                        } else if (locTokens.size() == 2 && locTokens[0] == "autoindex") {
                            if (locTokens[1] != "on" && locTokens[1] != "off") {
                                throw std::runtime_error("Invalid autoindex value in location: " + locTokens[1]);
                            }
                            loc_directives.setAutoIndex(locTokens[1]);
                        } else if (locTokens.size() > 1 && locTokens[0] == "allowed_methods") {
                            if (locTokens.size() > 4 || locTokens.size() < 2) {
                                throw std::runtime_error("Invalid allowed_methods directive in location block: " + locLine);
                            }
                            std::string allowedMethods;
                            for (size_t i = 1; i < locTokens.size(); ++i) {
                                allowedMethods += locTokens[i] + " ";
                            }
                            if (!allowedMethods.empty()) {
                                allowedMethods = allowedMethods.substr(0, allowedMethods.length() - 1);
                            }
                            loc_directives.setAllowedMethods(allowedMethods);
                        } else if (locTokens.size() == 2 && locTokens[0] == "upload_dir") {
                            loc_directives.setUploadDir(locTokens[1]);
                        } else if (locTokens.size() == 3 && locTokens[0] == "cgi") {
                            loc_directives.setCgiExtension(locTokens[1]);
                            loc_directives.setCgiPath(locTokens[2]);
                        } else if (locTokens.size() == 3 && locTokens[0] == "return") {
                            loc_directives.setReturn(locTokens[1] + " " + locTokens[2]);
                        } else if (locTokens[0] == "listen" || locTokens[0] == "server_name" ||
                                   locTokens[0] == "client_max_body_size" || locTokens[0] == "error_page") {
                            throw std::runtime_error("Directive '" + locTokens[0] + "' is only allowed in server block, not in location block");
                        } else {
                            throw std::runtime_error("Unknown directive in location block: " + locTokens[0]);
                        }
                    }
                    s_directives.addLocation(loc_directives);
                } else if (blockTokens[0] == "upload_dir" || blockTokens[0] == "return" ||
                           blockTokens[0] == "cgi") {
                    throw std::runtime_error("Directive '" + blockTokens[0] + "' is only allowed in location block, not in server block");
                } else {
                    throw std::runtime_error("Unknown directive in server block: " + blockTokens[0]);
                }
            }
            _servers.push_back(parse_server_directives(s_directives));
        }
    }

    file.close();
}

#include "includes/ConfigParser.hpp" 

ConfigParser::ConfigParser(const std::string& path) : _configFile(path) {}


std::vector<ServerConfig> ConfigParser::getServers() const {
    return _servers;
}
#include <iostream>
#include <vector>
#include <string>

int	is_delimiter(char c, const std::string& delimiters)
{
	size_t	i;

	i = 0;
	while (i < delimiters.length())
	{
		if (c == delimiters[i])
			return (1);
		i++;
	}
	return (0);
}

std::vector<std::string> split(const std::string& str,
const std::string& dlocConfigelimiters)
{
	std::vector<std::string>	result;

	size_t	start;
	size_t	i;

	start = 0;
	i = 0;

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

	return (result);
}


void ConfigParser::parse() {
    std::ifstream file(_configFile);
    if (!file.is_open()) {
        throw std::runtime_error("Could not open config file: " + _configFile);
    }

    std::string line;
    while (std::getline(file, line)) {
        std::vector<std::string> tokens = split(line, " \t");
        if (tokens.size() > 0 && tokens[0][0] == '#')
            continue; // Skip comments

        if (tokens.contains("server") && tokens.contains("{") && tokens.size() == 2) {
            serverDirectives s_directives;
            std::string blockLine;
            while (std::getline(file, blockLine)) {
                std::vector<std::string> blockTokens = split(blockLine, " \t");
                if (blockLine.start_with("#"))
                    continue; // Skip comments
                if (blockLine == "}") {
                    break; // End of server block
                }
                if (blockTokens[-1] == ";") {
                    blockTokens.pop_back();
                } else if (blockTokens[-1].end_with(";")) {
                    blockTokens[-1] = blockTokens[-1].substr(0, blockTokens[-1].length() - 1);
                } else {
                    throw std::runtime_error("Missing semicolon at the end of directive: " + blockLine);
                }
                if (blockTokens.size() == 2 && blockTokens[0] == "listen") {
                    s_directives.setListen(blockTokens[1]);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "server_name") {
                    s_directives.setServerName(blockTokens[1]);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "root") {
                    s_directives.setRoot(blockTokens[1]);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "index") {
                    s_directives.setIndex(blockTokens[1]);
                } else if (blockTokens.size() > 1 && blockTokens[0] == "error_page") {
                    if (blockTokens.size() != 3) {
                        throw std::runtime_error("Invalid error_page directive: " + blockLine);
                    }
                    std::string errorPage = blockTokens[1] + " " + blockTokens[2];
                    s_directives.addErrorPage(errorPage);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "autoindex") {
                    s_directives.setAutoIndex(blockTokens[1]);
                } else if (blockTokens.size() > 1 && blockTokens[0] == "allowed_methods") {
                    if(blockTokens.size() > 4 || blockTokens.size() < 2) {
                        throw std::runtime_error("Invalid allowed_methods directive: " + blockLine);
                    }
                    std::string allowedMethods;
                    for(size_t i = 1; i < blockTokens.size(); ++i) {
                        allowedMethods += blockTokens[i] + " ";
                    }                    allowedMethods.pop_back(); // Remove trailing space
                    s_directives.setAllowedMethods(allowedMethods);
                    // s_directives.setAllowedMethods(blockTokens[1]);
                } else if (blockTokens.size() == 2 && blockTokens[0] == "client_max_body_size") {
                    s_directives.setClientMaxBodySize(blockTokens[1]);
                }
                else if (blockTokens.size() == 2 && blockTokens[0] == "location") {
                    locationDirectives loc_directives;
                    loc_directives.setPath(blockTokens[1]);
                    std::string locLine;
                    while (std::getline(file, locLine)) {
                        std::vector<std::string> locTokens = split(locLine, " \t");
                        if (locLine.start_with("#"))
                            continue; // Skip comments
                        if (locLine == "}") {
                            break; // End of location block
                        }
                        if (locTokens[-1] == ";") {
                            locTokens.pop_back();
                        } else if (locTokens[-1].end_with(";")) {
                            locTokens[-1] = locTokens[-1].substr(0, locTokens[-1].length() - 1);
                        } else {
                            throw std::runtime_error("Missing semicolon at the end of directive: " + locLine);
                        }
                        if (locTokens.size() == 2 && locTokens[0] == "root") {
                            loc_directives.setRoot(locTokens[1]);
                        } else if (locTokens.size() == 2 && locTokens[0] == "index") {
                            loc_directives.setIndex(locTokens[1]);
                        } else if (locTokens.size() == 2 && locTokens[0] == "autoindex") {
                            loc_directives.setAutoIndex(locTokens[1]);
                        } else if (locTokens.size() > 1 && locTokens[0] == "allowed_methods") {
                            if (locTokens.size() > 4 || locTokens.size() < 2) {
                                throw std::runtime_error("Invalid allowed_methods directive in location block: " + locLine);
                            }
                            std::string allowedMethods;
                            for (size_t i = 1; i < locTokens.size(); ++i) {
                                allowedMethods += locTokens[i] + " ";
                            }
                            allowedMethods.pop_back(); // Remove trailing space
                            loc_directives.setAllowedMethods(allowedMethods);
                        } else if (locTokens.size() == 2 && locTokens[0] == "upload_dir") {
                            loc_directives.setUploadDir(locTokens[1]);
                        } else if (locTokens.size() > 1 && locTokens[0] == "cgi") {
                            if (locTokens.size() != 3) {
                                throw std::runtime_error("CGI directive in location block missing parameters");
                            }
                            loc_directives.setCgiExtension(locTokens[1]);
                            loc_directives.setCgiPath(locTokens[2]);
                        } else if (locTokens.size() > 1 && locTokens[0] == "return") {
                            if (locTokens.size() != 3) {
                                throw std::runtime_error("Return directive in location block missing parameters");
                            }
                            loc_directives.setReturn(locTokens[1] + " " + locTokens[2]);
                        }
                         else {
                            throw std::runtime_error("Unknown directive in location block: " + locTokens[0]);
                        }
                    }
                    s_directives.addLocation(loc_directives);
                } else {
                    throw std::runtime_error("Unknown directive in server block: " + blockTokens[0]);
                }
            }
            _servers.push_back(parse_server_directives(s_directives));
        }
    }

    file.close();
}

serverConfig ConfigParser::parse_server_directives(const serverDirectives& s_directives) {
    ServerConfig config;
    locationDirectives defaultLocation;

    // Parse root directive
    defaultLocation.setRoot(s_directives.getRoot());
    // Parse index directive
    defaultLocation.setIndex(s_directives.getIndex());
    // Parse cgiExtension and cgiPath directives
    defaultLocation.setCgiExtension(s_directives.getCgiExtension());
    defaultLocation.setCgiPath(s_directives.getCgiPath());
    defaultLocation.setAutoIndex(s_directives.getAutoIndex());
    defaultLocation.setAllowedMethods(s_directives.getAllowedMethods());
    // Parse listen directive
    if (s_directives.getListen().empty()) {
        throw std::runtime_error("Listen directive is required in server block");
    }
    std::vector<std::string> listenParts = split(s_directives.getListen(), ":");
    if (listenParts.size() == 0 || listenParts.size() > 2) {
        throw std::runtime_error("Invalid listen directive: " + s_directives.getListen());
    }
    if (listenParts.size() == 1) {
        config.setPort(std::stoi(listenParts[0]));
    }
    else {
        config.setHost(listenParts[0]);
        config.setPort(std::stoi(listenParts[1]));
    }
    // Parse server_name directive
    if (!s_directives.getServerName().empty()) {
        config.setServerName(s_directives.getServerName());
    }
    // client_max_body_size directive
    if (!s_directives.getClientMaxBodySize().empty()) {
        config.setClientMaxBodySize(std::stoul(s_directives.getClientMaxBodySize()));
    }
    // Parse error_page directive
    for (size_t i = 0; i < s_directives.getErrorPages().size(); ++i) {
        std::vector<std::string> errorParts = split(s_directives.getErrorPages()[i], " ");
        if (errorParts.size() != 2) {
            throw std::runtime_error("Invalid error_page directive: " + s_directives.getErrorPages()[i]);
        }
        int statusCode = std::stoi(errorParts[0]);
        config.addErrorPage(statusCode, errorParts[1]);
    }
    //

    // Parse location blocks
    for (size_t i = 0; i < s_directives.getLocations().size(); ++i) {
        const locationDirectives& locDir = s_directives.getLocations()[i];
        LocationConfig locConfig;
        locConfig.setPath(locDir.getPath());
        if (locDir.getRoot().empty()) {
            locConfig.setRoot(defaultLocation.getRoot());
        } else {
            locConfig.setRoot(locDir.getRoot());
        }
        if (locDir.getIndex().empty()) {
            locConfig.setIndex(defaultLocation.getIndex());
        } else {
            locConfig.setIndex(locDir.getIndex());
        }
        locConfig.setAutoIndex(defaultLocation.getAutoIndex()=="on" ? true : false);
        if(locDir.getAutoIndex() == "on") {
            locConfig.setAutoIndex(true);
        } else if (locDir.getAutoIndex() == "off") {
            locConfig.setAutoIndex(false);
        }
        std::vector<std::string> allowedMethods = split(defaultLocation.getAllowedMethods(), " ");
        locConfig.setAllowedMethods(allowedMethods);
        if(!locDir.getAllowedMethods().empty()) {
            allowedMethods = split(locDir.getAllowedMethods(), " ");
            locConfig.setAllowedMethods(allowedMethods);
        }
        if (!locDir.getUploadDir().empty()) {
            locConfig.setUploadDir(locDir.getUploadDir());
        }
        if (!locDir.getReturn().empty()) {
            int statusCode;
            std::string url;
            std::vector<std::string> returnParts = split(locDir.getReturn(), " ");
            if (returnParts.size() != 2) {
                throw std::runtime_error("Invalid return directive in location block: " + locDir.getReturn());
            }
            statusCode = std::stoi(returnParts[0]);
            url = returnParts[1];
            locConfig.setReturn(statusCode, url);
        }
        if (!locDir.getCgiExtension().empty() && !locDir.getCgiPath().empty()) {
            locConfig.setCgiExtension(locDir.getCgiExtension());
            locConfig.setCgiPath(locDir.getCgiPath());
        }
        config.getLocations().push_back(locConfig);
    }
    return config;
}
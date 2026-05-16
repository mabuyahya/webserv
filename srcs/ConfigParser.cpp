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
const std::string& delimiters)
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
0               std::vector<std::string> blockTokens = split(blockLine, " \t");
                if (blockLine.start_with("#"))
                    continue; // Skip comments
                if (blockLine == "}") {
                    break; // End of server block
                }
                if (blockTokens.size() > 0 && blockTokens[0] == "listen") {
                    s_directives.setListen(blockTokens[1]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "server_name") {
                    s_directives.setServerName(blockTokens[1]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "root") {
                    s_directives.setRoot(blockTokens[1]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "index") {
                    s_directives.setIndex(blockTokens[1]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "error_page") {
                    s_directives.setErrorPage(blockTokens[1]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "cgi") {
                    s_directives.setCgiExtension(blockTokens[1]);
                    s_directives.setCgiPath(blockTokens[2]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "autoindex") {
                    s_directives.setAutoIndex(blockTokens[1]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "allowed_methods") {
                    s_directives.setAllowedMethods(blockTokens[1]);
                } else if (blockTokens.size() > 0 && blockTokens[0] == "client_max_body_size") {
                    s_directives.setClientMaxBodySize(blockTokens[1]);
                }
                else if (blockTokens.size() > 0 && blockTokens[0] == "location") {
                    locationDirectives loc_directives;
                    if (blockTokens.size() < 2) {
                        throw std::runtime_error("Location directive missing path");
                    }
                    loc_directives.setPath(blockTokens[1]);
                    std::string locLine;
                    while (std::getline(file, locLine)) {
                        std::vector<std::string> locTokens = split(locLine, " \t");
                        if (locLine == "}") {
                            break; // End of location block
                        }
                        if (locLine.start_with("#"))
                            continue; // Skip comments
                        if (locTokens.size() > 0 && locTokens[0] == "root") {
                            loc_directives.setRoot(locTokens[1]);
                        } else if (locTokens.size() > 0 && locTokens[0] == "index") {
                            loc_directives.setIndex(locTokens[1]);
                        } else if (locTokens.size() > 0 && locTokens[0] == "autoindex") {
                            loc_directives.setAutoIndex(locTokens[1]);
                        } else if (locTokens.size() > 0 && locTokens[0] == "allowed_methods") {
                            loc_directives.setAllowedMethods(locTokens[1]);
                        } else if (locTokens.size() > 0 && locTokens[0] == "upload_dir") {
                            loc_directives.setUploadDir(locTokens[1]);
                        } else if (locTokens.size() > 0 && locTokens[0] == "cgi") {
                            if (locTokens.size() < 3) {
                                throw std::runtime_error("CGI directive in location block missing parameters");
                            }
                            loc_directives.setCgiExtension(locTokens[1]);
                            loc_directives.setCgiPath(locTokens[2]);
                        } else {
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
    // Parse listen directive
    std::vector<std::string> listenParts = split(s_directives.getListen(), ":");
    if (s_directives.getListen().empty()) {
        throw std::runtime_error("Listen directive is required in server block");
    }
    if (listenParts.size() == 0 || listenParts.size() > 2) {
        throw std::runtime_error("Invalid listen directive: " + s_directives.getListen());
    }
    if (listenParts.size() == 1) {
        config.setPort(std::stoi(listenParts[0]));
    } else {
        config.setHost(listenParts[0]);
        config.setPort(std::stoi(listenParts[1]));
    }

    // Parse server_name directive
    config.setServerName(s_directives.getServerName());

    // Parse root directive
    config.setRoot(s_directives.getRoot());

    // Parse index directive
    config.setIndex(s_directives.getIndex());

    // Parse error_page directive
    if (!s_directives.getErrorPage().empty()) {
        std::vector<std::string> errorParts = split(s_directives.getErrorPage(), " ");
        if (errorParts.size() != 2) {
            throw std::runtime_error("Invalid error_page directive: " + s_directives.getErrorPage());
        }
        int statusCode = std::stoi(errorParts[0]);
        config.addErrorPage(statusCode, errorParts[1]);
    }

    // Parse CGI directives
    if (!s_directives.getCgiExtension().empty() && !s_directives.getCgiPath().empty()) {
        config.setCgiExtension(s_directives.getCgiExtension());
        config.setCgiPath(s_directives.getCgiPath());
    } else if (!s_directives.getCgiExtension().empty() || !s_directives.getCgiPath().empty()) {
        throw std::runtime_error("Both cgi_extension and cgi_path must be set for CGI configuration");
    }
    // Parse autoindex directive
    if (!s_directives.getAutoIndex().empty()) {
        config.setAutoIndex(s_directives.getAutoIndex() == "on");
    }
    // Parse allowed_methods directive
    if (!s_directives.getAllowedMethods().empty()) {
        config.setAllowedMethods(split(s_directives.getAllowedMethods(), " "));
    }
    // Parse client_max_body_size directive
    if (!s_directives.getClientMaxBodySize().empty()) {
        config.setClientMaxBodySize(std::stoul(s_directives.getClientMaxBodySize()));
    }
    // Parse location blocks
    for (size_t i = 0; i < s_directives.getLocations().size(); ++i) {
        const locationDirectives& locDir = s_directives.getLocations()[i];
        LocationConfig locConfig;
        locConfig.setPath(locDir.getPath());
        locConfig.setRoot(locDir.getRoot());
        locConfig.setIndex(locDir.getIndex());
        locConfig.setAutoIndex(locDir.getAutoIndex() == "on");
        locConfig.setAllowedMethods(split(locDir.getAllowedMethods(), " "));
        locConfig.setUploadDir(locDir.getUploadDir());
        if (!locDir.getCgiExtension().empty() && !locDir.getCgiPath().empty()) {
            locConfig.setCgiExtension(locDir.getCgiExtension());
            locConfig.setCgiPath(locDir.getCgiPath());
        }
        config.getLocations().push_back(locConfig);
    }
    return config;
}
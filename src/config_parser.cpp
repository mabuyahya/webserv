#include "config_parser.hpp"
#include <sstream>

ConfigParser::ConfigParser(const std::string& filename) : filename(filename)
{
}

ConfigParser::~ConfigParser()
{
}

ConfigParser::ConfigParser(const ConfigParser& other) : filename(other.filename)
{
	this->configs = other.configs;
	this->tokens = other.tokens;
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		filename = other.filename;
		configs = other.configs;
		tokens = other.tokens;
	}
	return *this;
}

LocationBlock parseLocationBlock(std::vector<std::string> &tokens, size_t &i)
{
	LocationBlock location;

	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == "root")
		{
			i++;
			location.root = tokens[i];
			i++;
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after root directive");
			}
		}
		else if (tokens[i] == "index")
		{
			i++;
			location.index = tokens[i];
			i++;
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after index directive");
			}
		}
		else if (tokens[i] == "allow_methods")
		{
			i++;
			while (i < tokens.size() && tokens[i] != ";")
			{
				location.allow_methods.push_back(tokens[i]);
				i++;
			}
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after allow_methods directive");
			}
		}
		else if (tokens[i] == "autoindex")
		{
			i++;
			if (tokens[i] == "on") {
				location.autoindex = true;
			} else if (tokens[i] == "off") {
				location.autoindex = false;
			} else {
				throw std::runtime_error("Syntax error: autoindex must be 'on' or 'off'");
			}
			i++;
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after autoindex directive");
			}
		}
		else if (tokens[i] == "storage")
		{
			i += 3; // skip storage and the path for now
			continue;
		}
		else if (tokens[i] == "cgi_pass")
		{
			i += 3; // skip cgi_pass and the path for now
			continue;
		}
		else if (tokens[i] == "return")
		{
			i += 4; // skip return and the path for now
			continue;
		}
		else
		{
			throw std::runtime_error("Unknown directive in location block: " + tokens[i]);
		}
		i++;
	}

	if (i >= tokens.size()) {
		throw std::runtime_error("Syntax error: missing closing '}' for location block");
	}

	return location;
}

ServerConfig parseServerBlock(std::vector<std::string> &tokens, size_t &i)
{
	ServerConfig server;

	while (i < tokens.size() && tokens[i] != "}")
	{
		if (tokens[i] == "listen")
		{
			i++;
			server.host = tokens[i];
			i++;
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after listen directive");
			}
		}
		else if (tokens[i] == "location")
		{
			i++;
			std::string path = tokens[i];
			i++;
			if (i >= tokens.size() || tokens[i] != "{") {
				throw std::runtime_error("Syntax error: expected '{' after location path");
			}
			i++;

			LocationBlock new_location = parseLocationBlock(tokens, i);
			new_location.path = path;
			server.locations.push_back(new_location);
		}
		else if (tokens[i] == "server_name")
		{
			i++;
			server.server_name = tokens[i];
			i++;
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after server_name directive");
			}
		}
		else if (tokens[i] == "client_max_body_size")
		{
			// TODO check for size types like 1k, 1m, 1g
			i++;
			server.client_max_body_size = std::atoi(tokens[i].c_str());
			i++;
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after client_max_body_size directive");
			}
		}
		else if (tokens[i] == "error_page")
		{
			i++;
			int error_code = std::atoi(tokens[i].c_str());
			i++;
			server.error_pages[error_code] = tokens[i];
			i++;
			if (i >= tokens.size() || tokens[i] != ";") {
				throw std::runtime_error("Syntax error: missing ';' after error_page directive");
			}
		}
		else
		{
			throw std::runtime_error("Unknown directive in server block: " + tokens[i]);
		}
		i++;
	}

	if (i >= tokens.size()) {
		throw std::runtime_error("Syntax error: missing closing '}' for server block");
	}
	i++; // skip the closing '}'
	return server;
}

std::vector<ServerConfig> ConfigParser::parseConfig()
{
	std::ifstream file(filename.c_str());
	std::vector<std::string> tokens;
	std::vector<ServerConfig> configs;


	if (!file.is_open())
	{
		throw std::runtime_error("Could not open config file: " + filename);
	}

	std::string content;
	std::string curr_line;

	while (std::getline(file, curr_line))
	{
		// strip out comments cuz we will have a lot of them
		size_t comment_pos = curr_line.find('#');
		if (comment_pos != std::string::npos)
		{
			curr_line = curr_line.substr(0, comment_pos);
		}
		content += curr_line + "\n";
	}

	std::string padded_content;
	for (size_t i = 0; i < content.length(); ++i) {
		if (content[i] == '{' || content[i] == '}' || content[i] == ';') {
			padded_content += " ";
			padded_content += content[i];
			padded_content += " ";
		} else {
			padded_content += content[i];
		}
	}

	std::istringstream iss(padded_content);
	std::string token;
	while (iss >> token)
	{
		tokens.push_back(token);
	}
	// TODO move the data into the config struct and return it

	size_t i = 0;
	/*
		this while loop expects the config file to be in this format:
		server {
		stuff
		}
		server {
		stuff2
		}
	*/
	while (i < tokens.size())
	{
		if (tokens[i] == "server")
		{
			i++;
			if (i >= tokens.size() || tokens[i] != "{")
			{
				throw std::runtime_error("Expected '{' after 'server'");
			}
			i++;

			ServerConfig new_server = parseServerBlock(tokens, i);
			configs.push_back(new_server);
		}
		else
		{
			std::cerr << "Unexpected token: " << tokens[i] << std::endl;
			throw std::runtime_error("Expected 'server' block");
		}
	}
	return configs;
}

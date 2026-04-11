#include "config_parser.hpp"
#include <sstream>

ConfigParser::ConfigParser(const std::string& filename) : filename(filename)
{
}

ConfigParser::~ConfigParser()
{
}

ConfigParser::ConfigParser(const ConfigParser& other) : filename(other.filename), config(other.config)
{
}

ConfigParser& ConfigParser::operator=(const ConfigParser& other)
{
	if (this != &other)
	{
		filename = other.filename;
		config = other.config;
	}
	return *this;
}

ServerConfig ConfigParser::parseConfig()
{
	std::ifstream file(filename.c_str());
	std::vector<std::string> tokens;

	if (!file.is_open())
	{
		throw new std::runtime_error("Could not open config file: " + filename);
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
		std::cout << "Token: " << token << std::endl;
	}
	// TODO move the data into the config struct and return it
	return config;
}

#include <iostream>
#include "include/config_parser.hpp"

int main(int argc, char* argv[])
{
	/* ! the subject says:
		Your program must use a configuration file, provided as an argument on the com-mand line,
		or available in a default path.
		So this check may be unnecessary.
	*/
	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <Config File>" << std::endl;
		return 1;
	}

	// * Config parsing
	try {
		ConfigParser parser(argv[1]);
		ServerConfig config = parser.parseConfig();
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return 1;
	}
	// * Start the server and listen

	return 0;
}

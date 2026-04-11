#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << "الشباب هترسب مليون بالمية" << std::endl;

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

	// * Start the server and listen

	return 0;
}

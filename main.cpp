#include <iostream>

int main(int argc, char* argv[])
{
	std::cout << "الشباب هترسب مليون بالمية" << std::endl;

	if (argc < 2)
	{
		std::cerr << "Usage: " << argv[0] << " <Config File>" << std::endl;
		return 1;
	}

	return 0;
}

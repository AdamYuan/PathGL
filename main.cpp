#include <cstring>
#include <stdexcept>
#include "Application.hpp"

int main(int argc, char **argv)
{
	argc --; argv ++;

	unsigned width{1440}, height{900}, invocation_size_x{5}, invocation_size_y{5};
	try
	{
		for(int i = 0; i < argc; i += 2)
		{
			char *cmd = argv[i];
			char *arg = argv[i + 1];
			if(strcmp(cmd, "-w") == 0)
				width = (unsigned)std::stoul(arg);
			else if(strcmp(cmd, "-h") == 0)
				height = (unsigned)std::stoul(arg);
			else if(strcmp(cmd, "-ix") == 0)
				invocation_size_x = (unsigned)std::stoul(arg);
			else if(strcmp(cmd, "-iy") == 0)
				invocation_size_y = (unsigned)std::stoul(arg);
			else
				throw std::runtime_error("invalid argument " + std::string(cmd));
		}
	}
	catch (std::exception &e)
	{
		printf("error: %s\n", e.what());
		printf("usage: ./PathGL -w [width] -h [height] -ix [invocation size x] -iy [invocation size y]\n");
		return EXIT_FAILURE;
	}


	Application app(width, height, invocation_size_x, invocation_size_y);
	app.Run();

	return EXIT_SUCCESS;
}
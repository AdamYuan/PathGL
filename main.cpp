#include <cstring>
#include <stdexcept>
#include "Application.hpp"

int main(int argc, char **argv)
{
	argc --; argv ++;

	unsigned width{1440}, height{900}, invocation_size_x{20}, invocation_size_y{20}, samples_per_calculation{1},
			max_depth{4};
	float fov = 45.0f, speed = 0.1f, angle = 2.5f;
	const char *scene_filename{nullptr};
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
			else if(strcmp(cmd, "-cf") == 0)
				fov = std::stof(arg);
			else if(strcmp(cmd, "-cs") == 0)
				speed = std::stof(arg);
			else if(strcmp(cmd, "-ca") == 0)
				angle = std::stof(arg);
			else if(strcmp(cmd, "-scn") == 0)
				scene_filename = arg;
			else if(strcmp(cmd, "-spc") == 0)
				samples_per_calculation = (unsigned)std::stoul(arg);
			else if(strcmp(cmd, "-md") == 0)
				max_depth = (unsigned)std::stoul(arg);
			else
				throw std::runtime_error("invalid argument " + std::string(cmd));
		}
		if(!scene_filename)
			throw std::runtime_error("no scene file");
	}
	catch (std::exception &e)
	{
		printf("error: %s\n", e.what());
		printf("usage: ./PathGL \n"
			   "\t-scn [scene file name]\n"
	  		   "\t-md [max depth]\n"
			   "\t-w [width] -h [height]\n"
			   "\t-ix [invocation size x] -iy [invocation size y]\n"
			   "\t-cf [fov (degree)] -cs [camera speed] -ca [rotate angle (degree)]\n"
			   "\t-spc [samples per calculation(default is 1)]\n"
		);
		return EXIT_FAILURE;
	}


	Application app(samples_per_calculation, max_depth,
					width, height, invocation_size_x, invocation_size_y, fov, speed, angle,
					scene_filename);
	app.Run();

	return EXIT_SUCCESS;
}
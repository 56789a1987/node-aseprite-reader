#include <stdio.h>
#include <fstream>
#include "../src/aseprite-reader.h"

int main(int argc, char **argv)
{
	AsepriteReader reader;

	try
	{
		uint32_t size;
		std::ifstream in;
		in.open("test.aseprite");
		in.seekg(0, std::ios::end);
		size = in.tellg();
		in.seekg(0, std::ios::beg);
		char *buffer = new char[size];
		in.read(buffer, size);
		in.close();

		reader.load((uint8_t *)buffer, size);
	}
	catch (const std::exception &e)
	{
		printf("Fail: %s\n", e.what());
		return 1;
	}

	printf("Tags:\n");
	for (auto &tag : reader.file.tags)
	{
		printf(" - %s\n", tag->name.c_str());
	}

	printf("Layers:\n");
	for (auto &layer : reader.file.layers)
	{
		printf(" - %s\n", layer->name.c_str());
	}

	printf("Palette:\n");
	for (auto &color : reader.file.palette->colors)
	{
		printf("\e[48;2;%d;%d;%dm  \e[0m", color->r, color->g, color->b);
	}
	printf("\n");

	printf("Success\n");
	return 0;
};

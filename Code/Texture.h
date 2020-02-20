#pragma once
#include "BufferCPU.h"
#include "TextureGPU.h"
#include "Demo.h"

// forward declaration
class Demo;

class Texture
{
public:
	int width;
	int height;

	// Texture data
	BufferCPU* textureCPU;
	TextureGPU* textureGPU;

	Texture(char* file);
	~Texture();

	static uint32_t CountMips(uint32_t width, uint32_t height)
	{
		if (width == 0 || height == 0 || width != height)
			return 0;

		uint32_t count = 1;
		while (width > 1 && height > 1)
		{
			width >>= 1;
			height >>= 1;
			count++;
		}
		return count;
	}
};


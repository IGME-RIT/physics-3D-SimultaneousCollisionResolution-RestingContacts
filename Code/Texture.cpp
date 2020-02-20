#include "Texture.h"
#include "Helper.h"

#define STBI_ONLY_PNG
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Texture::Texture(char* file)
{
	Demo* demo = Demo::GetInstance();

	// This may be hard to believe, but there
	// are some GPUs out there that do not support
	// image textures. These GPUs are, of course, not
	// any kind of consumer-level GPU, but we still should
	// check the GPu to see if it supports textures, just
	// in case. Here we create VkFormatProperties to 
	// check to see if the GPU supports the texture
	// format we want
	VkFormatProperties props;

	// This is the texture format we want, it 
	// is very simple, it has a red channel, a green
	// channel, a blue channel, and an alpha channel
	const VkFormat tex_format = VK_FORMAT_R8G8B8A8_UNORM;

	// Check the properties that the GPU supports for this 
	// texture format. If the GPU does not support this format,
	// then the format properties it returns will be NULL
	vkGetPhysicalDeviceFormatProperties(demo->gpu, tex_format, &props);

	// The process of using a sampler (which we just made) to get
	// pixels from the texture, is caled "sampling pixels from the image".

	// If our GPU has the ability to sample pixels from
	// this image format (RGBA), then we can load our texture
	if (props.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_BIT)
	{
		// create a pointer to our png data.
		// This is not the pixel data, this is
		// an array that holds every byte of
		// the PNG texture file
		char* pngData;

		// create an int to store the size
		int png_size;

		// the number of channels that the image
		// has, honestly just ignore it, we won't 
		// use it, but we need to have it
		int nchan;

		// create variables for the width and
		// the height of the texture we load
		int tex_width;
		int tex_height;

		// Use a helper function to read the file. 
		// Every pixel in the flie will be copied into the pngData
		// array of bytes, and the size of the array will go into png_size
		Helper::ReadFile(file, &pngData, &png_size);

		// Use STB Image to texture data from the PNG bytes.
		// I personally use STB becasue it is lightweight and it
		// works on every platform that I develop for. It works
		// with OpenGL, DirectX 11 / 12, Xbox One, Switch, PlayStation, and more

		// We give it the pngData, and the size,
		// then the function gives us (via pointers) the texture's width and height,
		// as well as the number of channels, which we ignore. The function returns
		// a pointer (stored into 'img'), which is the pointer to the pixel data that
		// the PNG holds
		stbi_uc* img = stbi_load_from_memory((const stbi_uc*)pngData, png_size, &tex_width, &tex_height, &nchan, 4);

		// now that the image is loaded, we don't need the original file data
		// we delete pngData, and use 'img', the pointer to our pixels
		delete pngData;

		// We are going to combine what we did to create the Vertex / Index buffers,
		// with what we did to create the depth buffers. We need to create a CPU
		// buffer to hold the pixel data, then we need to create an identical GPU
		// buffer that is empty, then we need to copy the pixel data from the CPU
		// buffer to the GPU, so that it can be used by the shaders

		// save the data for later
		width = tex_width;
		height = tex_height;

		// information we need to create the buffer
		VkBufferCreateInfo info = {};
		info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;	// sType always needs to be this
		info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;		// this is a source, which will be copied to the GPU
		info.size = tex_width * tex_height * 4;				// amount of bytes in the image

		textureCPU = new BufferCPU(demo->device, demo->memory_properties, info);
		textureCPU->Store(img, (int)info.size);

		// after copying this to the buffer
		// we don't need the original,
		stbi_image_free(img);

		// get the number of mips for this image
		int mipLevelCount = CountMips(width, height);

		// The CPU buffer is ready, now we need the GPU buffer

		// Just like when we created the depth buffer,
		// we have a VkImageCreateInfo. We give it the
		// required sType, we let it know that it is a 2D
		// image, we give it the texture dimensions, we 
		// let it know that this is PREINITIALIZED, which means
		// that the GPU buffer  is currently empty 
		VkImageCreateInfo image_create_info = {};
		image_create_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		image_create_info.imageType = VK_IMAGE_TYPE_2D;
		image_create_info.format = tex_format;
		image_create_info.extent.width = tex_width;
		image_create_info.extent.height = tex_height;
		image_create_info.extent.depth = 1;
		image_create_info.mipLevels = mipLevelCount > 0 ? mipLevelCount : 1;
		image_create_info.arrayLayers = 1;
		image_create_info.samples = VK_SAMPLE_COUNT_1_BIT;
		image_create_info.tiling = VK_IMAGE_TILING_OPTIMAL;
		image_create_info.usage = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
		image_create_info.initialLayout = VK_IMAGE_LAYOUT_PREINITIALIZED;

		if (image_create_info.mipLevels > 1)
			image_create_info.usage |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;

		// We create the GPU texture the same way that
		// we created the depth buffer, except this time,
		// we use the Color aspect instead of depth
		textureGPU = new TextureGPU(
			demo->device,
			tex_width, tex_height,
			demo->memory_properties,
			image_create_info,
			VK_IMAGE_ASPECT_COLOR_BIT);

		// We put a command in the command buffer that we want to copy an image
		// from the CPU to the GPU. This command will execute when we submit
		// the initCmd command buffer to the queue, which happens later in prepare().
		// It works the same way as normal buffers, except we give it the texture
		// parameters, and a few extra steps are required under-the-hood in the 
		// TextureGPU class. Students don't need to understand how TextureGPU works,
		// but they can try to learn it if they want to. What is important is that
		// they know how to use the class.
		textureGPU->Store(demo->initCmd, textureCPU, tex_width, tex_height);
	}

	// If the GPU does not support the ability to sample
	// pixels from this texture format with our sampler, 
	// then give an error that says the format is not supported
	else
	{
		assert(!"No support for R8G8B8A8_UNORM as texture image format");
	}
}


Texture::~Texture()
{
	delete textureGPU;
}

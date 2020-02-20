#pragma once
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>

class Pipe
{
public:
	// pipeline data
	VkPipeline pipeline;
	VkPipelineCache pipelineCache;
	VkPipelineLayout pipeline_layout;
	
	// needed to delete
	VkDevice device;

	Pipe();
	~Pipe();
};


#pragma once
#include "Pipe.h"

class PipeSky : public Pipe
{
public:
	PipeSky(VkDevice dev, VkDescriptorSetLayout desc_layout, VkRenderPass render_pass);
	~PipeSky();
};


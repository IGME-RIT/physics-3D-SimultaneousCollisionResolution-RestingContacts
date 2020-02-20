#pragma once
#include "Pipe.h"

class PipeBumpy : public Pipe
{
public:
	PipeBumpy(VkDevice dev, VkDescriptorSetLayout desc_layout, VkRenderPass render_pass);
	~PipeBumpy();
};


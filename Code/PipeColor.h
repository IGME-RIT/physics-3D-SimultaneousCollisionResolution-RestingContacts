#pragma once
#include "Pipe.h"

class PipeColor : public Pipe
{
public:
	PipeColor(VkDevice dev, VkDescriptorSetLayout desc_layout, VkRenderPass render_pass);
	~PipeColor();
};


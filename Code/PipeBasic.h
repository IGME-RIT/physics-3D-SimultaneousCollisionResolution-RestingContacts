#pragma once
#include "Pipe.h"

class PipeBasic : public Pipe
{
public:
	PipeBasic(VkDevice dev, VkDescriptorSetLayout desc_layout, VkRenderPass render_pass);
	~PipeBasic();
};


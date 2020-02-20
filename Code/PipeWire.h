#pragma once
#include "Pipe.h"

class PipeWire : public Pipe
{
public:
	PipeWire(VkDevice dev, VkDescriptorSetLayout desc_layout, VkRenderPass render_pass);
	~PipeWire();
};


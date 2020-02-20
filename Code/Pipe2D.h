#pragma once
#include "Pipe.h"

class Pipe2D : public Pipe
{
public:
	Pipe2D(VkDevice dev, VkDescriptorSetLayout desc_layout, VkRenderPass render_pass);
	~Pipe2D();
};




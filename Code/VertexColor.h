#pragma once
#include <vulkan/vulkan.h>

// The structure of each Vertex
// Each vertex has a position
// and a color
struct VertexColor
{
	// X, Y, and Z axis for 3D position
	float position[3];

	// R, G, and B, for color
	float color[3];

	// get the state
	static VkPipelineVertexInputStateCreateInfo GetState();
};


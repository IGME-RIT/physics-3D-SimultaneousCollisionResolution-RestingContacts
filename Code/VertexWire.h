#pragma once
#include <vulkan/vulkan.h>

// The structure of each Vertex
// Each vertex has a position
struct VertexWire
{
	// X, Y, and Z axis for 3D position
	float position[3];

	// get the state
	static VkPipelineVertexInputStateCreateInfo GetState();
};


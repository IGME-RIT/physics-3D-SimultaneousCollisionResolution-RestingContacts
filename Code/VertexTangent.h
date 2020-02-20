#pragma once
#include <vulkan/vulkan.h>

// The structure of each Vertex
// Each vertex has a position
// a UV texture coordinate, a
// normal, and a tangent
struct VertexTangent
{
	// X, Y, and Z axis for 3D position
	float position[3];

	// X and Y, for 2D Texture coordinate
	float uv[2];

	// X, Y, and Z directions for normal
	float normal[3];

	// X, Y, and Z directions for tangent
	float tangent[3];

	// get the state
	static VkPipelineVertexInputStateCreateInfo GetState();
};
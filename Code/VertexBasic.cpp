#include "VertexBasic.h"

// three attributes
static VkVertexInputAttributeDescription vertexInputAttributs[3];

// binding description
static VkVertexInputBindingDescription vertexInputBinding = {};

// vertex input createinfo
static VkPipelineVertexInputStateCreateInfo vi = {};

VkPipelineVertexInputStateCreateInfo VertexBasic::GetState()
{
	// Vertex input binding
	// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
	// The binding description says that vertices will be given to the GPU, one at a time,
	// and we tell it how large each Vertex is. If the GPU has a Vertex Buffer that is 100kb large,
	// the GPU needs to know where each vertex starts and ends, and it knows that by knowing
	// how large each vertex is, which we tell it here
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = sizeof(VertexBasic);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Input attribute bindings describe shader attribute locations and memory layouts
	// This is very similar to how vertex attributes work in any other. Basically,
	// in the last structure, we say how large each vertex is, but in this structure,
	// we say how large each piece of the vertex is
	memset(vertexInputAttributs, 0, sizeof(VkVertexInputAttributeDescription) * 3);

	// location = 0, because this is the first element of the vertex
	vertexInputAttributs[0].location = 0;

	// offsetof returns 0, because position starts 0 bytes after the start of the vertex
	vertexInputAttributs[0].offset = offsetof(VertexBasic, position);

	// format is R32-G32-B32, because there are three 32-bit floats in the position data,
	// one float for X, one float for Y, and one float for Z, of each position, of each vertex
	vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;

	// location = 1, because this is the second element of the vertex
	vertexInputAttributs[1].location = 1;

	// offsetof returns 12, because uv starts 12 bytes after the start of the vertex,
	// because there are 12 bits in position (3 floats that are 4 bytes large)
	vertexInputAttributs[1].offset = offsetof(VertexBasic, uv);

	// format is R32-G32, because there are two floats in every UV.
	vertexInputAttributs[1].format = VK_FORMAT_R32G32_SFLOAT;

	// location = 2, because this is third element of the vertex
	vertexInputAttributs[2].location = 2;

	// offsetof returns 20, because normal starts 20 bytes after the start of the vertex,
	// because there are 20 bits in position and UV combined
	vertexInputAttributs[2].offset = offsetof(VertexBasic, normal);

	// format is R32-G32-B32, because there are three floats in every normal.
	vertexInputAttributs[2].format = VK_FORMAT_R32G32B32_SFLOAT;

	// Vertex Input State
	// This combines the last two structures we made
	// We give it the requires sType, we give it the number of 
	// binding descriptions, which is one, and we give it the 
	// pointer to the bindingInput, because it is not an array.
	// We tell it how many attributes there are (three) (pos, uv, and normal),
	// then we give it the array of two attributes (which is already a pointer)
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vi.vertexBindingDescriptionCount = 1;
	vi.pVertexBindingDescriptions = &vertexInputBinding;
	vi.vertexAttributeDescriptionCount = 3;
	vi.pVertexAttributeDescriptions = vertexInputAttributs;

	return vi;
}
#include "VertexWire.h"

// one attributes
static VkVertexInputAttributeDescription vertexInputAttributs[1];

// binding description
static VkVertexInputBindingDescription vertexInputBinding = {};

// vertex input createinfo
static VkPipelineVertexInputStateCreateInfo vi = {};

VkPipelineVertexInputStateCreateInfo VertexWire::GetState()
{
	// Vertex input binding
	// This example uses a single vertex input binding at binding point 0 (see vkCmdBindVertexBuffers)
	// The binding description says that vertices will be given to the GPU, one at a time,
	// and we tell it how large each Vertex is. If the GPU has a Vertex Buffer that is 100kb large,
	// the GPU needs to know where each vertex starts and ends, and it knows that by knowing
	// how large each vertex is, which we tell it here
	vertexInputBinding.binding = 0;
	vertexInputBinding.stride = sizeof(VertexWire);
	vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	// Input attribute bindings describe shader attribute locations and memory layouts
	// This is very similar to how vertex attributes work in any other. Basically,
	// in the last structure, we say how large each vertex is, but in this structure,
	// we say how large each piece of the vertex is
	memset(vertexInputAttributs, 0, sizeof(VkVertexInputAttributeDescription) * 1);

	// location = 0, because this is the first element of the vertex
	vertexInputAttributs[0].location = 0;

	// offsetof returns 0, because position starts 0 bytes after the start of the vertex
	vertexInputAttributs[0].offset = offsetof(VertexWire, position);

	// format is R32-G32-B32, because there are three 32-bit floats in the position data,
	// one float for X, one float for Y, and one float for Z, of each position, of each vertex
	vertexInputAttributs[0].format = VK_FORMAT_R32G32B32_SFLOAT;

	// Vertex Input State
	// This combines the last two structures we made
	// We give it the requires sType, we give it the number of 
	// binding descriptions, which is one, and we give it the 
	// pointer to the bindingInput, because it is not an array.
	// We tell it how many attributes there are (two) (pos, color),
	// then we give it the array of two attributes (which is already a pointer)
	vi.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vi.vertexBindingDescriptionCount = 1;
	vi.pVertexBindingDescriptions = &vertexInputBinding;
	vi.vertexAttributeDescriptionCount = 1;
	vi.pVertexAttributeDescriptions = vertexInputAttributs;

	return vi;
}
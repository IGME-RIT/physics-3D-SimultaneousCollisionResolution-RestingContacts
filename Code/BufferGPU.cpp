/*
Copyright 2019
Original authors: Niko Procopi
Written under the supervision of David I. Schwartz, Ph.D., and
supported by a professional development seed grant from the B. Thomas
Golisano College of Computing & Information Sciences
(https://www.rit.edu/gccis) at the Rochester Institute of Technology.
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
<http://www.gnu.org/licenses/>.

Special Thanks to Exzap from Team Cemu,
he gave me advice on how to optimize Vulkan
graphics, he is working on a Wii U emulator
that utilizes Vulkan, see more at http://cemu.info
*/

#include "BufferGPU.h"
#include "Helper.h"
#include <vector>

// the cache system here is identical to 
// the one in TextureCPU

// save pointers to all CPU buffers that 
// are queued to cpoy into GPU, so that
// we can delete them after they copy to GPU
static std::vector<BufferCPU*> cpuCache;

// clear CPU buffers after they are copeid to GPU
// This will be called in Demo.cpp
void BufferGPU::ClearCacheCPU()
{
	// delete all the memory allocated in RAM.
	// Now that it is in GPU, we dont need it in RAM
	for (int i = 0; i < cpuCache.size(); i++)
		delete cpuCache[i];

	// clear the vector, because we dont need
	// to keep a record of deleted buffers
	cpuCache.clear();
}

// When we create a GPU buffer, we need the Device (lets us give commands to GPU),
// we need the Memory Properties from the PhysicalDevice (same GPU, but the properties of teh GPU),
// and we need the BufferCreateInfo, to tell us what type of buffer this is (uniform, vertex, index, etc)
BufferGPU::BufferGPU(VkDevice d, VkPhysicalDeviceMemoryProperties memory_properties, VkBufferCreateInfo info)
{
	// save device, so that
	// we can use it to store
	// data, and delete data
	device = d;

	// create buffer with the device,
	// and the VkBufferCreateInfo
	vkCreateBuffer(device, &info, NULL, &buffer);

	// get memory requirements, so that we know
	// what we need in order to allocate the memory
	VkMemoryRequirements mem_reqs;
	vkGetBufferMemoryRequirements(device, buffer, &mem_reqs);

	// create allocation information
	// this uses the memory requirements
	// to tell the device how to allocate the buffer
	VkMemoryAllocateInfo memAllocInfo = {};
	memAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memAllocInfo.allocationSize = mem_reqs.size;

	// This function works the same as it did when
	// we made a CPU Buffer, but this time it looks
	// for a memoryTypeIndex that lets us make
	// a buffer in the GPU's memory

	// find memory on the GPU's VRAM
	// VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT says we are in Device memory
	// Reminder: Device (vkDevice) references the graphics card
	Helper::memory_type_from_properties(
		memory_properties,
		mem_reqs.memoryTypeBits,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		&memAllocInfo.memoryTypeIndex);

	// allocate memory
	// our memoryAllocationInfo got the required size
	// from the VkMemoryRequirements, and it got
	// the type of memory from memory_type_from_properties
	vkAllocateMemory(device, &memAllocInfo, NULL, &memory);

	// After our memory is allocated, we have
	// to bind the buffer to the memory, so that
	// we can use the memory
	vkBindBufferMemory(device, buffer, memory, 0);
}

BufferGPU::~BufferGPU()
{
	// when we want to delete this CPU memory
	// we delete the buffer, which is what we
	// used to access the memory
	vkDestroyBuffer(device, buffer, NULL);

	// after deleting the buffer, there is no
	// way to access the memory, so we delete
	// the memory
	vkFreeMemory(device, memory, NULL);
}

void BufferGPU::Store(VkCommandBuffer cmd, BufferCPU* cpuBuffer, int size)
{
	// Storing memory on the GPU is different from
	// how we did it on the CPU. We need a command buffer
	// to copy memory from CPU to GPU. This cmd will be
	// the initialization command buffer from prepare().
	// This command buffer is started in prepare(), and it
	// will be executed before prepare() ends.

	// First we make a copyRegion, which does
	// not require an sType, to my surprise.
	// We give it the size of the buffer
	VkBufferCopy copyRegion = {};
	copyRegion.size = size;

	// We put a command into the command buffer, that
	// will copy memory from the CPU buffer (given in the
	// parameter) to the GPU Buffer. However, unlike
	// The CPU's "Store" function, the GPU's "Store"
	// function does not immediately transfer data from
	// CPU to GPU. This just puts the command into the command
	// buffer. Data will finally be copied from CPU to GPU
	// after the command buffer with this vkCmdCopy command,
	// is executed, which will happen before the end of prepare()
	vkCmdCopyBuffer(cmd, cpuBuffer->buffer, buffer, 1, &copyRegion);

	// keep a record of all buffers that are queued
	// to be copied to the GPU, so that they can be
	// deleted from RAM after they are copeid to GPU
	cpuCache.push_back(cpuBuffer);
}
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

#pragma once

#include "Entity.h"

#define APP_NAME_STR_LEN 80
#include <vulkan/vulkan.h>
#include <vulkan/vk_sdk_platform.h>
#include "BufferCPU.h"
#include "BufferGPU.h"
#include "TextureGPU.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Allow a maximum of two outstanding presentation operations.
#define FRAME_LAG 2

#include "PipeBumpy.h"
#include "PipeBasic.h"
#include "PipeColor.h"
#include "PipeSky.h"
#include "Pipe2D.h"
#include "PIpeWire.h"

typedef struct {
	VkImage image;
	VkImageView view;

	VkCommandBuffer cmd;
	VkFramebuffer framebuffer;
} SwapchainImageResources;

// Forward Declaration
// This is needed because Entity.h and Demo.h both include each other,
// the same is needed for the other classes, for the same reason
class Entity;
class Mesh;
class Texture;

class Demo
{
	static Demo* m_pInstance;

public:
	static Demo* GetInstance();

	bool* keys;

	char name[APP_NAME_STR_LEN];  // Name to put on the window/icon
	HWND window;                  // hWnd - window handle
	POINT minsize;                // minimum window size

	VkSurfaceKHR surface;
	bool prepared;
	bool is_minimized;

	bool syncd_with_actual_presents;
	uint64_t refresh_duration;
	uint64_t refresh_duration_multiplier;
	uint64_t target_IPD;  // image present duration (inverse of frame rate)
	uint64_t prev_desired_present_time;
	uint32_t next_present_id;
	uint32_t last_early_id;  // 0 if no early images
	uint32_t last_late_id;   // 0 if no late images

	VkInstance inst;
	VkPhysicalDevice gpu;
	VkDevice device;
	VkQueue queue;
	VkSemaphore image_acquired_semaphores[FRAME_LAG];
	VkSemaphore draw_complete_semaphores[FRAME_LAG];
	VkPhysicalDeviceMemoryProperties memory_properties;

	VkPhysicalDeviceFeatures supportedFeatures;
	VkPhysicalDeviceFeatures desiredFeatures;

	uint32_t enabled_extension_count;
	uint32_t enabled_layer_count;
	char *extension_names[64];
	char *enabled_layers[64];

	int width, height;
	VkFormat format;
	VkColorSpaceKHR color_space;

	// matrices used for all entities
	glm::mat4x4 projection_matrix;
	glm::mat4x4 view_matrix;

	// Function pointers that we get from the instance
	PFN_vkGetPhysicalDeviceSurfaceSupportKHR fpGetPhysicalDeviceSurfaceSupportKHR;
	PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR fpGetPhysicalDeviceSurfaceCapabilitiesKHR;
	PFN_vkGetPhysicalDeviceSurfaceFormatsKHR fpGetPhysicalDeviceSurfaceFormatsKHR;
	PFN_vkGetPhysicalDeviceSurfacePresentModesKHR fpGetPhysicalDeviceSurfacePresentModesKHR;
	PFN_vkGetSwapchainImagesKHR fpGetSwapchainImagesKHR;
	PFN_vkGetDeviceProcAddr fpGetDeviceProcAddr;

	// Function pointers that we get from the device
	PFN_vkCreateSwapchainKHR fpCreateSwapchainKHR;
	PFN_vkDestroySwapchainKHR fpDestroySwapchainKHR;
	PFN_vkAcquireNextImageKHR fpAcquireNextImageKHR;
	PFN_vkQueuePresentKHR fpQueuePresentKHR;

	// swapchain, and the swapchain images
	VkSwapchainKHR swapchain;
	uint32_t swapchainImageCount;
	SwapchainImageResources *swapchain_image_resources;

	// current buffer index that is being used
	uint32_t current_buffer;

	// current mode of the swapchain
	VkPresentModeKHR currentPresentMode;

	// fences that are used for drawing
	VkFence drawFences[FRAME_LAG];
	int frame_index;

	// Init Fence
	VkFence initFence;

	// sampler that is used by all textures
	VkSampler sampler;

	// Depth Buffer
	TextureGPU* depthBufferGPU;

	// command buffers
	VkCommandPool cmd_pool;
	VkCommandBuffer initCmd;
	VkCommandBuffer drawCmd;

	// Descriptors
	VkDescriptorPool desc_pool;
	VkDescriptorSetLayout desc_layout_color;
	VkDescriptorSetLayout desc_layout_basic;
	VkDescriptorSetLayout desc_layout_bumpy;
	VkDescriptorSetLayout desc_layout_wire;

	// renderpass
	VkRenderPass render_pass;

	// Pipelines
	PipeBumpy* pipeBumpy;
	PipeBasic* pipeBasic;
	PipeColor* pipeColor;
	PipeSky* pipeSky;
	Pipe2D* pipe2D;
	PipeWire* pipeWire;

	bool validate;

	void prepare_console();
	void prepare_window();
	void prepare_instance();
	void prepare_physical_device();
	void prepare_instance_functionPointers();
	void prepare_surface();
	void prepare_device_queue();
	void prepare_device_functionPointers();
	void prepare_synchronization();
	void prepare_swapchain();
	void prepare_init_cmd();
	void begin_init_cmd();
	void prepare_sampler();
	void prepare_descriptor_layout();
	void prepare_descriptor_pool();
	void prepare_depth_buffer();
	void execute_init_cmd();
	void prepare_render_pass();
	void prepare_framebuffers();
	void build_swapchain_cmds();
	void ResetEntityList();
	void ApplyPipeline2D();
	void ApplyPipelineColor();
	void ApplyPipelineBasic();
	void ApplyPipelineBumpy();
	void ApplyPipelineSky();
	void ApplyPipelineWire();
	void DrawEntity(Entity* e);
	void build_secondary_cmd();
	void prepare();


	void delete_resolution_dependencies();
	void resize();
	void draw();
	void run();

	Demo();
	~Demo();
};


#include "PipeSky.h"
#include "VertexBasic.h"

// This is exactly the same as PipeBasic,
// except for the shaders. With this setup,
// anything related to the pipeline can be changed

PipeSky::PipeSky(VkDevice dev, VkDescriptorSetLayout desc_layout, VkRenderPass render_pass)
{
	device = dev;

	// Now we create a pipeline layout, which will have
	// one descriptor set in it. Super simple, just use
	// the required sType, give it the layout we just made,
	// and let it know that there is one descriptor layout
	VkPipelineLayoutCreateInfo pPipelineLayoutCreateInfo = {};
	pPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pPipelineLayoutCreateInfo.setLayoutCount = 1;
	pPipelineLayoutCreateInfo.pSetLayouts = &desc_layout;
	
	// Make the layout, we will use this when we build the pipeline later on
	vkCreatePipelineLayout(device, &pPipelineLayoutCreateInfo, NULL, &pipeline_layout);

	// This is the CreateInfo for full pipeline
	// This will be the largest CreateInfo structure of the
	// entire Vulkan program, so get ready for it

	// first we give the pipeline our required sType, then
	// we give it the pipeline layout and the render pass 
	// that we made earlier
	VkGraphicsPipelineCreateInfo pipeInfo = {};
	pipeInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipeInfo.layout = pipeline_layout;
	pipeInfo.renderPass = render_pass;

	VkPipelineVertexInputStateCreateInfo vi = VertexBasic::GetState();

	// we put the InputStateCrateInfo into the PipelineCreateInfo
	pipeInfo.pVertexInputState = &vi;

	// All that, and now the pipeline knows how to take in
	// each vertex as input. Next, we will tell the GPU what
	// to do with the vertices. In thsi case, we want to draw 
	// a list of trangles with the vertices. This will connect
	// vertices 0, 1, and 2 together, then it will connect 3, 4,
	// and 5 toghether, and so on

	// Input Assembly State
	// we give it the required sType, and then we tell it
	// that we want to draw triangles, but you could also draw
	// points or lines if someone wanted to try that. In this case,
	// we set Topology to TRIANGLE_LIST, so that we can draw triangles
	VkPipelineInputAssemblyStateCreateInfo ia = {};
	ia.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	ia.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

	// we give the InputAssemblyStateCreateInfo to the PipelineCreateInfo
	pipeInfo.pInputAssemblyState = &ia;

	// This is an of DynamicStates
	// This is part of the DynamicStateCreateInfo
	// which goes into PipelineCreateInfo. We want
	// two types of dynamic states, we want the
	// viewport and the scissor, which we set in
	// the command buffers that draw our scene
	VkDynamicState dynamicStateEnables[VK_DYNAMIC_STATE_RANGE_SIZE];
	memset(dynamicStateEnables, 0, sizeof dynamicStateEnables);
	dynamicStateEnables[0] = VK_DYNAMIC_STATE_VIEWPORT;
	dynamicStateEnables[1] = VK_DYNAMIC_STATE_SCISSOR;

	// This is the dynamic state
	// we give it the requierd sType, we tell it that
	// we have two dynamicStates, and we give the array
	// of dynamic states that we have
	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStateEnables;

	// we give our DynamicStateCreateInfo to the PipelineCreateInfo
	pipeInfo.pDynamicState = &dynamicState;

	// We tell Vulkan how many viewports and scissors there are.
	// I know this feels repetitive, it is, but after we set this
	// up, we don't need to change it. We will change some parts of
	// the pipeline after this in future tutorials, but we won't change
	// anything to do with viewport and scissor

	// give it the sType it requires
	// let it know that there is one viewport and one scissor
	VkPipelineViewportStateCreateInfo vp = {};
	vp.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	vp.viewportCount = 1;
	vp.scissorCount = 1;

	// give the viewportStateCreateInfo to the PipelineCreateInfo
	pipeInfo.pViewportState = &vp;

	// This structure togggles all the options in the rasterizer,
	// which is what converts polygons into pixels, so that they can
	// be shaded in the fragment shader. There are around 10 different
	// variables that we can configure with the rasterizer, but for
	// now we will keep it simple, we give it the required sType,
	// we say that we want the polygons to be filled (opposed to lines),
	// we cull the back sides of the polygon faces, so that only the
	// front sides of the polygons are drawn. If you don't know what that does
	// try replacing VK_CULL_MODE_BACK_BIT with VK_CULL_MODE_FRONT_BIT, and then
	// only the front sides of the polygon will be drawn. Alternatively, if you
	// want both sides of the polygons to be drawn (which is useless when 
	// drawing a cube), try VK_CULL_MODE_NONE_BIT.
	// We set the line width to 1.0f, even though we aren't drawing lines,
	// because Vulkan validator will yell at us if we don't
	VkPipelineRasterizationStateCreateInfo rs = {};
	rs.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rs.polygonMode = VK_POLYGON_MODE_FILL;
	rs.cullMode = VK_CULL_MODE_BACK_BIT;
	rs.lineWidth = 1.0f;

	// give the RasterizationInfo to the PipelineCreateInfo
	pipeInfo.pRasterizationState = &rs;

	// one of these is needed for every render target
	// there is one in this case, which is the swapchain,
	// so we create an array of one blendAttachmentState

	// this handles transparency. In this case, we do not
	// want transparency, so we set blending to disable,
	// and we set the colorWrite to 0xF, which is the maximum
	// value, so the polygons are 100% drawn.
	VkPipelineColorBlendAttachmentState att_state[1];
	memset(att_state, 0, sizeof(att_state));
	att_state[0].colorWriteMask = 0xF;
	att_state[0].blendEnable = VK_FALSE;

	// create the blend attachment state, which holds
	// all of our blendStateAttachments. In this case, we
	// give it the requierd sType, we tell it there is one
	// member of the array of attachments, and we give it the
	// array of one attachment
	VkPipelineColorBlendStateCreateInfo cb = {};
	cb.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	cb.attachmentCount = 1;
	cb.pAttachments = att_state;

	// give the blendState to the PipelineCreateInfo
	pipeInfo.pColorBlendState = &cb;

	// Now that we've gone through all the necessary 
	// struggle to make a depth buffer, allocate data on 
	// the GPU for it, make a vkImageView to make it accessible,
	// and setup our renderpass + framebuffer to use the depth buffer,
	// we can finally tell the computer what to do with 
	// the depth buffer. Here we create the DepthStateCreateInfo

	// First we give it the necessary sType,
	// we set DepthTest to true, because we want to compare depth
	// of each pixel to the depth buffer, we enable writing to the
	// depth buffer, we use the LESS_OR_EQUAL operation, so that
	// new pixels are drawn to the screen if they are closer to 
	// the camera than any pixel that is already rendered (or equal),
	// then we let the pipeline know that we are always comparing depths
	VkPipelineDepthStencilStateCreateInfo ds = {};
	ds.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	ds.depthTestEnable = VK_TRUE;
	ds.depthWriteEnable = VK_TRUE;
	ds.depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
	ds.back.compareOp = VK_COMPARE_OP_ALWAYS;

	// give the depthState to the PipelineCreateInfo
	pipeInfo.pDepthStencilState = &ds;

	// multisample state
	// this allows for multisample anti-aliasing (MSAA).
	// In this tutorial, we won't be using MSAA, so we set
	// the number of samples to 1, and we give the necessary sType.
	// If someone wants MSAA, it takes a little more effort than
	// changing COUNT-1-BIT to COUNT-8-BIT, but this is definetely
	// where it starts, there may be a tutorial on MSAA later
	VkPipelineMultisampleStateCreateInfo ms = {};
	ms.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	ms.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

	// give multisample state to the PipelineCreateInfo
	pipeInfo.pMultisampleState = &ms;

	// Time for shaders, and there is a lot to say about shaders.
	// The way I've set up shaders is I've written the Vertex
	// and Fragment shaders into two files: cube.vert and cube.frag.

	// If you want to have GLSL code inside the CPP file, and then
	// compile the GLSL code at runtime, you can do that with a 
	// library called "shaderc". This library is actually already
	// built-in to the visual studio solution, linked and ready 
	// to go. Here is a tutorial if anyone is interested:
	// https://www.reddit.com/r/vulkan/comments/bbuh0p/compiling_glsl_in_shader_rather_than_precompiling/

	// If you don't want to do that, there are two more options.

	// 1.You can precompile shaders, and load the shaders
	// from files at runtime.
	// 2. You can precompile shaders, and then turn the shader files
	// into an array of bytes, and compile those bytes into the 
	// EXE file. This way, the shaders are compiled AND inside the
	// program, so the end-user won't have any shader files.

	// Right now, I have written a program called "compileShaders.cmd"
	// First, it compiles the GLSL files cube.vert and cube.frag into
	// compiled shader files:
	//		..\Bin\glslangValidator.exe -V cube.vert -o cube.vert.spv
	// Next, it optimizes the compiled shader by removing the 
	// debug features of the shader. This can be disabled if you want
	//		..\Bin\spirv-opt --strip-debug cube.vert.spv -o cube2.vert.spv

	// If you want to load precompiled shaders from files at runtime,
	// you can use cube2.vert.spv and cube2.frag.spv. However,
	// you will not find these files anywhere, and I will explain why 
	// in a second.

	// What we do next is, take the optimized compiled shader files,
	// and turn them into an array of bytes
	//		bin2hex --i cube2.vert.spv --o cube.vert.inc
	// Then, after it is converted to an array of bytes,
	// the original compiled shader files are deleted
	//		del cube.vert.spv
	//		del cube2.vert.spv

	// Finally, the array of bytes are included here.
	// If you open the inc files in notepad, you will
	// see the bytes of the compiled shader, looks like
	// this:
	//	0x03, 0x02, 0x23, 0x07, 0x00, 0x00, 0x01, ...

	// By using #include, we can put those 
	// arrays into the C++ program, and then
	// we can compile the arrays into the EXE

	// Vertex Shader compiled to header
	const unsigned char vs_code[] = {
		#include "sky.vert.inc"
	};

	// Fragment Shader compiled to header
	const unsigned char fs_code[] = {
		#include "sky.frag.inc"
	};

	// If you do not want to do this ^^^
	// if you would prefer to take the compiled shader files
	// and load them at runtime, you can make an empty array
	// of bytes: char* vs_code = nullptr, char* fs_code = nullptr,
	// Then you can use Helper::ReadFile to laod data from the 
	// comopiled shader file into the byte arrays when the program
	// launches, it works exactly the same as reading the PNG texture
	// file, like we did before. If you want to use compiled shader
	// files, then delete the lines in the compileShaders.cmd file
	// that say:
	//		del cube.vert.spv
	//		del cube2.vert.spv

	// Create two shader modules
	VkShaderModule vert_shader_module;
	VkShaderModule frag_shader_module;

	// Now, we need CreateInfo for each Shader Module.
	// A Shader Module is a piece of a total Shader Program.
	// One Shader Program is a combination of a vertex shader,
	// a pixel shader, and sometimes more. So an individual
	// vertex shader is a shader module, and a fragment shader
	// is a shader module. We make a CreateInfo with the required sType
	VkShaderModuleCreateInfo shaderInfo = {};
	shaderInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

	// We give a pointer to the bytes of the compiled vertex shader,
	// and the number of bytes that are in the compiled vertex shader
	shaderInfo.pCode = (uint32_t*)vs_code;
	shaderInfo.codeSize = sizeof(vs_code);

	// Then we use the createInfo to make the shader module
	vkCreateShaderModule(device, &shaderInfo, NULL, &vert_shader_module);

	// We are going to re-use the createInfo that we used for the
	// vertex shader, to make the createInfo for the fragment shader.
	// All we have to do is replace the vertex shader data with
	// the fragment shader data

	// we give the pointer to compiled fragment shader bytes
	// and the number of bytes in the compiled fragment shader
	shaderInfo.pCode = (uint32_t*)fs_code;
	shaderInfo.codeSize = sizeof(fs_code);

	// Then we use the createInfo to make the shader module
	vkCreateShaderModule(device, &shaderInfo, NULL, &frag_shader_module);

	// We create a list of pipeline stages
	// In this case, there are two stages, a vertex shader
	// and a fragment shader. We make the array, and use
	// memset to set all bytes in the array to zero
	VkPipelineShaderStageCreateInfo shaderStages[2];
	memset(&shaderStages, 0, sizeof(VkPipelineShaderStageCreateInfo) * 2);

	// First we give the required sType, then
	// we declare that thish particular stage is
	// a vertex shader (VERTEX_BIT), then we give
	// the vertex shader module that we made earlier,
	// and we let the pipeline know that the entry
	// function is "main". Rather than having 
	// "void main() {...}", you can actually call the 
	// function anything, as long as it matches the 
	// name that is given here
	shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
	shaderStages[0].module = vert_shader_module;
	shaderStages[0].pName = "main";

	// First we give the required sType, then
	// we declare that thish particular stage is
	// a fragment shader (FRAGMENT_BIT), then we give
	// the fragment shader module that we made earlier,
	// and we let the pipeline know that the entry
	// function is "main".
	shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	shaderStages[1].module = frag_shader_module;
	shaderStages[1].pName = "main";

	// we let the PipelienCreateInfo know that there are two
	// shader stages, and we give it the array of stages
	// that we just made a moment ago
	pipeInfo.stageCount = 2;
	pipeInfo.pStages = shaderStages;

	// Now that we have completed the PipelineCreateInfo,
	// we want to create a PipelineCache, so that we
	// can cache the pipeline after we create it. Caching
	// the pipeline allows the GPU to use the pipeline more
	// effeciently. We don't need to use the cache anywhere,
	// we just need to create it, and the Vulkan driver will
	// use the cache automatically, which is the only thing
	// that the driver really does automatically for us

	// We have a CacheCreateInfo with the required sType,
	// and that is all that we need for now
	VkPipelineCacheCreateInfo cacheInfo = {};
	cacheInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;

	// We create an empty pipeline cache object, based on 
	// the information given, which isn't a lot of information but
	// still required to make it work
	vkCreatePipelineCache(device, &cacheInfo, NULL, &pipelineCache);

	// create the pipeline, with our pipeInfo structure
	// and then our pipeline is stored into the cache
	vkCreateGraphicsPipelines(device, pipelineCache, 1, &pipeInfo, NULL, &pipeline);

	// Now that our shaders are now copied into the pipeline,
	// we do not need the individual modules anymore.
	// destroy shader modules, now that they aren't needed
	vkDestroyShaderModule(device, frag_shader_module, NULL);
	vkDestroyShaderModule(device, vert_shader_module, NULL);
}

PipeSky::~PipeSky()
{
}

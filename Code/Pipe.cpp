#include "Pipe.h"



Pipe::Pipe()
{
}


Pipe::~Pipe()
{
	// We destroy the pipeline data
	vkDestroyPipeline(device, pipeline, NULL);
	vkDestroyPipelineCache(device, pipelineCache, NULL);
	vkDestroyPipelineLayout(device, pipeline_layout, NULL);
}

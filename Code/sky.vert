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

#version 450

layout (location = 0) in vec3 inPos;
layout (location = 1) in vec2 inUV;
layout (location = 2) in vec3 inNormal;

layout (std140, binding = 0) uniform bufferVals {
    mat4 projection;
	mat4 view;
	mat4 model;
};

layout (location = 0) out vec2 outUV;

void main() 
{	
	outUV = inUV;
	
	// ignore model matrix, and ignore position of camera
	// by turning mat4(view) into mat3(view).
	// If you don't know why that works, look at our
	// Skybox tutorial for OpenGL
	vec4 p = projection * view * model * vec4(inPos, 1);
	
	// Instead of outputting p, we will swizzle the output to send x y w w.
	// The gpu will end up dividing z by w to get depth between 0 and 1.
	// By making z = w, we make the result of that division 1, the maximum possible depth.
	// This makes it so that the skybox renders only where there is absolutely nothing else.
	gl_Position = p.xyww;
}
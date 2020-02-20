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
layout (location = 3) in vec3 inTangent;

layout (std140, binding = 0) uniform bufferVals {
    mat4 projection;
	mat4 view;
	mat4 model;
};

layout (location = 0) out vec2 outUV;
layout (location = 1) out vec3 outPos;
layout (location = 2) out vec3 outNormal;
layout (location = 3) out vec3 outTangent;
layout (location = 4) out vec3 outBitangent;

void main() 
{	
	outUV = inUV;
	
	// position in world space
	vec4 p = model * vec4(inPos, 1);
	outPos = p.xyz;

	// normal in world space, ignoring translation
	vec3 n = mat3(model) * inNormal;
	outNormal = normalize(n.xyz);
	
	// tangent in world space, ignoring translation
	vec3 t = mat3(model) * inTangent;
	outTangent = normalize(t.xyz);

	// The third vector we need is a bitangent, or a vector perpendicular to both the normal and tangent.
	// This can be easily accomplished with a cross product.
	outBitangent = normalize(cross(outTangent, outNormal));

	gl_Position = projection * view * p;
}
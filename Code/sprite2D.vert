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

// No vertices, no vertex buffer and no index buffer

layout (std140, binding = 0) uniform bufferVals {
	mat4 mdlvMtx;
	int textureWidth;
	int textureHeight;
	int numCharacters;
	float aspectRatio; // not used yet
	uniform int text[100];
};

layout (location = 0) out vec2 vtxUV;

void main()
{
	vec2 uv;
	if (gl_VertexIndex == 0) uv = vec2(1, 1);
	if (gl_VertexIndex == 1) uv = vec2(0, 1);
	if (gl_VertexIndex == 2) uv = vec2(1, 0);
	if (gl_VertexIndex == 3) uv = vec2(0, 0);

	if (numCharacters == 0)
	{
		vtxUV = uv;
		vtxUV.y = 1-vtxUV.y;

		uv *= vec2(2);
		uv -= vec2(1);

		// scale coordinates by image
		uv *= vec2(textureWidth, textureHeight);

		// scale to prevent everything from being huge
		uv *= vec2(0.001, 0.001);

		// scale coordinates by aspect ratio
		uv.x /= aspectRatio;

		gl_Position = mdlvMtx * vec4(uv, 0.0, 1.0);
	}

	else
	{
		// gl_InstanceIndex

		// scale coordinates by image
		uv *= vec2(textureWidth, textureHeight);

		// scale to prevent everything from being huge
		uv *= vec2(0.001, 0.001);

		// scale coordinates by aspect ratio
		uv.x /= aspectRatio;

		// make square half as wide
		// make UV double as wide
		uv.x *= 0.5;

		uv.x += float(gl_InstanceIndex) * 0.072;

		gl_Position = mdlvMtx * vec4(uv, 0.0, 1.0);

		// should be char
		int letter = text[gl_InstanceIndex];

		int letterX = 0;
		int letterY = 0;

		letter -= 32;

		while (letter >= 10)
		{
			letter -= 10;
			letterY += 1;
		}

		letterX = letter;

		// Multiply by 0.1, because there are
		// 10 letters in each row and column.
		// Therefore, each letter is 0.1 UV wide,
		// and 0.1 UV tall, because that's 10%, or 1/10

		if (gl_VertexIndex == 0)
		{
			uv.x = (float(letterX) + 0.5f) * 0.1f;
			uv.y = (float(letterY) + 0.0f) * 0.1f;
		}

		if (gl_VertexIndex == 1)
		{
			uv.x = (float(letterX) + 0.0f) * 0.1f;
			uv.y = (float(letterY) + 0.0f) * 0.1f;
		}

		if (gl_VertexIndex == 2)
		{
			uv.x = (float(letterX) + 0.5f) * 0.1f;
			uv.y = (float(letterY) + 1.0f) * 0.1f;
		}

		if (gl_VertexIndex == 3)
		{
			uv.x = (float(letterX) + 0.0f) * 0.1f;
			uv.y = (float(letterY) + 1.0f) * 0.1f;
		}

		vtxUV = uv;
	}
}
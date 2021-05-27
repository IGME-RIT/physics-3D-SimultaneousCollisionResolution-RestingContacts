#include "Mesh.h"
#include "VertexBasic.h"
#include "VertexTangent.h"
#include "Demo.h"
#include <stdio.h>
#include <vector>

// Load vertices with position,
// uv, and normal
void Mesh::LoadBasic(char* file)
{
	Demo* demo = Demo::GetInstance();

	// Part 1
	// Initialize variables and pointers

	// array of vertices
	// This will stay NULL until part 3
	VertexBasic* vertList;

	// File that we open, the OBJ file
	FILE *f = fopen(file, "r");

	// This is temporary data, we will use this to hold
	// data that we get from each line of code
	float x[3];
	unsigned short y[9];

	// this holds all possible positions,
	// texture coordinates, and normals
	// that are scattered throughout the flie
	std::vector<float>pos;
	std::vector<float>uvs;
	std::vector<float>norms;

	// This tells us which position, uv, and normal
	// belong together to form each vertex
	std::vector <std::vector<unsigned short>> vertices;

	// This holds each line that we read
	char line[100];

	// We make a vector of indices
	// These indices will determine which vertices
	// to connect for each triangle. It will connect
	// the first three indices into a triangle, and 
	// then the next three, and so on.
	std::vector<uint16_t> indices;


	// Part 2
	// Fill vectors for pos, uvs, norms, and faces

	// Keep checking lines until we finish checking
	// every line in the file
	while (fgets(line, sizeof(line), f))
	{
		// Take in the position if you detect a 'v'
		if (sscanf(line, "v %f %f %f", &x[0], &x[1], &x[2]) == 3)
			for (int i = 0; i < 3; i++)
				pos.push_back(x[i]);

		// Take in the texture coordinate if youd etect 'vt
		if (sscanf(line, "vt %f %f", &x[0], &x[1]) == 2)
			for (int i = 0; i < 2; i++)
				uvs.push_back(x[i]);

		// take in a normal if you detect a 'vn'
		if (sscanf(line, "vn %f %f %f", &x[0], &x[1], &x[2]) == 3)
			for (int i = 0; i < 3; i++)
				norms.push_back(x[i]);

		// create a face with three vertices when you detect an 'f'
		if (sscanf(line, "f %hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu", &y[0], &y[1], &y[2], &y[3], &y[4], &y[5], &y[6], &y[7], &y[8]) == 9)
		{
			// three vertices in one face
			for (int i = 0; i < 3; i++)
			{
				// create a vector for each vertex
				std::vector<unsigned short> f;

				// use what is important to this vertex
				for (int j = 0; j < 3; j++)
					f.push_back(y[3 * i + j] - 1);

				// By default, our vertex is not found
				// in our vertex buffer, which means it will
				// be added to the vertex buffer as a new vertex
				bool found = false;

				// should be replaced with Binary Search
				// array should be in order of vertices[j][0]

				// This loop will check the entire vertex buffer,
				// every time it wants to add a vertex, to see if 
				// the vertex is already there. This is slow, but 
				// by removing repeating vertices in our vertex buffer,
				// it will take less time for the vertex shader to draw,
				// and draw our model more efficiently

				// If anyone finds a faster algorithm, PLEASE post it

				// If you want, you can remove this 'for' loop, and
				// the model will draw just the same as it does now,
				// except the frame-rate will take a hit, and your
				// vertex buffer will be larger. With one OBJ, it is not
				// noticable, but with 1000 OBJs, FPS does get impacted

				/*for (int j = 0; j < vertices.size(); j++)
				{
					if (vertices[j][0] == f[0])			// if positions are equal
						if (vertices[j][1] == f[1])		// if uv coords are equal
							if (vertices[j][2] == f[2]) // if normals are equal
							{
								// we found that the vertex already exists
								found = true;

								// add this index to the index buffer
								// to reuse this vertex
								indices.push_back(j);
								break;
							}
				}*/

				if (!found)
				{
					// add to the index buffer, give it the last
					// member of the current vertex buffer, because
					// we are putting our vertex at the end
					indices.push_back((int)vertices.size());

					// To Do:
					// do binary search to find where
					// this should be placed, so that
					// the vector in order of f[0]

					// For now:
					// Put it at the end
					vertices.push_back(f);
				}
			}
		}
	}

	// Part 3
	// Initialize more variables and pointers

	vertList = new VertexBasic[vertices.size()];

	// Part 4
	// Build Vertex List

	for (int i = 0; i < (int)vertices.size(); i++)
	{
		// make a blank vertex
		VertexBasic v;

		// Set the position of this vertex
		for (int k = 0; k < 3; k++)
		{
			int coordIndex = 3 * vertices[i][0] + k;
			v.position[k] = pos[coordIndex];
		}

		// Set the UV of this vertex
		for (int k = 0; k < 2; k++)
		{
			int uvIndex = 2 * vertices[i][1] + k;
			v.uv[k] = uvs[uvIndex];
		}

		// Set the normal of the vertex
		for (int k = 0; k < 3; k++)
		{
			int normalIndex = 3 * vertices[i][2] + k;
			v.normal[k] = norms[normalIndex];
		}

		// flip the Y axis of the UV
		// for some reason, UVs in 
		// all OBJs are upside-down
		v.uv[1] = 1 - v.uv[1];

		// put our vertex into the array
		vertList[i] = v;
	}

	// close the file that we opened
	fclose(f);

	// Part 6
	// Build our buffers

	// Create empty creationInfo
	// we will be re-using this several times
	// Leave it empty for now
	VkBufferCreateInfo info = {};

	// Vertex Buffer
	//=====================================

	// The size of our Vertex Array, will be the amount of 
	// elements (36) multiplied by the size of one vertex
	uint32_t vertexArraySize = (uint32_t)vertices.size() * sizeof(VertexBasic);

	// Lets make a buffer that is designed to be
	// a source (TRANSFER_SRC), that is large enough
	// to hold our vertex buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = vertexArraySize;

	// make the buffer, and store the data into the buffer
	// For more information on how this works, look at BufferCPU.cpp
	// Learning about BufferCPU is optional
	vertexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	vertexDataCPU->Store(vertList, vertexArraySize);

	// This next buffer will be on the GPU, it is designed to be a 
	// destination (TRANSFER_DST) for our data, and it will also be
	// a VERTEX_BUFFER, which allows us to bind this buffer as a
	// vertex buffer when we set up the pipeline
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = vertexArraySize;

	// build the buffer, and make a command to send the data from the CPU 
	// buffer to the GPU buffer. This command will be added to the initCmd
	// command buffer, and the data will finally be copied from CPU to GPU
	// when we execute the initCmd command buffer (later in prepare).
	// After that, we will delete the CPU buffer, becasue it won't be
	// needed once the data is copied to GPU.
	// For more information on how this works, look at BufferGPU.cpp
	// Learning about BufferGPU is optional
	vertexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	vertexDataGPU->Store(demo->initCmd, vertexDataCPU, vertexArraySize);

	// Index Buffer
	//=====================================

	// The size of this index array will be the number
	// of elements, multiplied by the size of one element
	uint32_t indexArraySize = (int)indices.size() * sizeof(uint16_t);

	// This is the Index CPU buffer, so we useTRANSFER_SRC 
	// because we will copy the buffer to GPU
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = indexArraySize;

	// Just like before, we make the buffer and we store data into it
	indexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	indexDataCPU->Store(indices.data(), indexArraySize);

	// This is the Index GPU buffer, so we use TRANSFER_DST 
	// because we will get data from CPU, just like the Vertex
	// GPU buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = indexArraySize;

	// build the buffer, and add a command to the initCmd command buffer
	// to copy data from the CPU buffer to the GPU buffer
	indexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	indexDataGPU->Store(demo->initCmd, indexDataCPU, indexArraySize);

	// This will be saved to the demo class
	numIndices = (int)indices.size();

	// initialize min and max
	for (int i = 0; i < 3; i++)
	{
		min[i] = vertList[0].position[i];
		max[i] = vertList[0].position[i];
	}

	// get min and max for collision
	for (int i = 1; i < (int)vertices.size(); i++)
	{
		// initialize min and max
		for (int j = 0; j < 3; j++)
		{
			if (vertList[i].position[j] < min[j]) min[j] = vertList[i].position[j];
			if (vertList[i].position[j] > max[j]) max[j] = vertList[i].position[j];
		}
	}

	// get halfwidth/center for collision
	for (int i = 0; i < 3; i++) {
		halfwidth[i] = (max[i] - min[i]) / 2.0f;
		center[i] = (max[i] + min[i]) / 2.0f;
	}
}

void Mesh::LoadTangents(char* file)
{
	Demo* demo = Demo::GetInstance();

	// Part 1
	// Initialize variables and pointers

	// array of vertices
	// This will stay NULL until part 3
	VertexTangent* vertList;

	// File that we open, the OBJ file
	FILE *f = fopen(file, "r");

	// This is temporary data, we will use this to hold
	// data that we get from each line of code
	float x[3];
	unsigned short y[9];

	// this holds all possible positions,
	// texture coordinates, and normals
	// that are scattered throughout the flie
	std::vector<float>pos;
	std::vector<float>uvs;
	std::vector<float>norms;

	// This tells us which position, uv, and normal
	// belong together to form each vertex
	std::vector <std::vector<unsigned short>> vertices;

	// This holds each line that we read
	char line[100];

	// We make a vector of indices
	// These indices will determine which vertices
	// to connect for each triangle. It will connect
	// the first three indices into a triangle, and 
	// then the next three, and so on.
	std::vector<uint16_t> indices;


	// Part 2
	// Fill vectors for pos, uvs, norms, and faces

	// Keep checking lines until we finish checking
	// every line in the file
	while (fgets(line, sizeof(line), f))
	{
		// Take in the position if you detect a 'v'
		if (sscanf(line, "v %f %f %f", &x[0], &x[1], &x[2]) == 3)
			for (int i = 0; i < 3; i++)
				pos.push_back(x[i]);

		// Take in the texture coordinate if youd etect 'vt
		if (sscanf(line, "vt %f %f", &x[0], &x[1]) == 2)
			for (int i = 0; i < 2; i++)
				uvs.push_back(x[i]);

		// take in a normal if you detect a 'vn'
		if (sscanf(line, "vn %f %f %f", &x[0], &x[1], &x[2]) == 3)
			for (int i = 0; i < 3; i++)
				norms.push_back(x[i]);

		// create a face with three vertices when you detect an 'f'
		if (sscanf(line, "f %hu/%hu/%hu %hu/%hu/%hu %hu/%hu/%hu", &y[0], &y[1], &y[2], &y[3], &y[4], &y[5], &y[6], &y[7], &y[8]) == 9)
		{
			// three vertices in one face
			for (int i = 0; i < 3; i++)
			{
				// create a vector for each vertex
				std::vector<unsigned short> f;

				// use what is important to this vertex
				for (int j = 0; j < 3; j++)
					f.push_back(y[3 * i + j] - 1);

				// By default, our vertex is not found
				// in our vertex buffer, which means it will
				// be added to the vertex buffer as a new vertex
				bool found = false;

				// In this algorithm, we need repeating vertices,
				// because one vertex that is shaerd in multiple trianges
				// might need a different tangent for each triangle

				if (!found)
				{
					// add to the index buffer, give it the last
					// member of the current vertex buffer, because
					// we are putting our vertex at the end
					indices.push_back((int)vertices.size());

					// To Do:
					// do binary search to find where
					// this should be placed, so that
					// the vector in order of f[0]

					// For now:
					// Put it at the end
					vertices.push_back(f);
				}
			}
		}
	}

	// Part 3
	// Initialize more variables and pointers

	vertList = new VertexTangent[vertices.size()];

	// Part 4
	// Build Vertex List

	for (int i = 0; i < (int)vertices.size(); i++)
	{
		// make a blank vertex
		VertexTangent v;

		// Set the position of this vertex
		for (int k = 0; k < 3; k++)
		{
			int coordIndex = 3 * vertices[i][0] + k;
			v.position[k] = pos[coordIndex];
		}

		// Set the UV of this vertex
		for (int k = 0; k < 2; k++)
		{
			int uvIndex = 2 * vertices[i][1] + k;
			v.uv[k] = uvs[uvIndex];
		}

		// Set the normal of the vertex
		for (int k = 0; k < 3; k++)
		{
			int normalIndex = 3 * vertices[i][2] + k;
			v.normal[k] = norms[normalIndex];
		}

		// flip the Y axis of the UV
		// for some reason, UVs in 
		// all OBJs are upside-down
		v.uv[1] = 1 - v.uv[1];

		// put our vertex into the array
		vertList[i] = v;
	}

	// The algorithm for tangents was copied, pasted, and adjusted
	// from the original tangents algorithm from 2001

	// This is the original source from 2001
	// http://www.terathon.com/code/tangent.html

	// Literally, this is THE source used by Unity, Unreal,
	// and every big AAA engine. This is what everyone uses 
	// for tangents

	// tangents
	// Calculate vectors relative to triangle positions
	
	// We do one interation of this loop per triangle,
	// so we divide the number of iterations by 3
	for (int i = 0; i < (int)vertices.size() / 3; i++)
	{

		float x1 = vertList[3 * i + 1].position[0] - vertList[3 * i + 0].position[0];
		float y1 = vertList[3 * i + 1].position[1] - vertList[3 * i + 0].position[1];
		float z1 = vertList[3 * i + 1].position[2] - vertList[3 * i + 0].position[2];

		float x2 = vertList[3 * i + 2].position[0] - vertList[3 * i + 0].position[0];
		float y2 = vertList[3 * i + 2].position[1] - vertList[3 * i + 0].position[1];
		float z2 = vertList[3 * i + 2].position[2] - vertList[3 * i + 0].position[2];

		// Do the same for vectors relative to triangle uv's
		float s1 = vertList[3 * i + 1].uv[0] - vertList[3 * i + 0].uv[0];
		float t1 = vertList[3 * i + 1].uv[1] - vertList[3 * i + 0].uv[1];

		float s2 = vertList[3 * i + 2].uv[0] - vertList[3 * i + 0].uv[0];
		float t2 = vertList[3 * i + 2].uv[1] - vertList[3 * i + 0].uv[1];

		// Create vectors for tangent calculation
		float r = 1.0f / (s1 * t2 - s2 * t1);

		float tx = (t2 * x1 - t1 * x2) * r;
		float ty = (t2 * y1 - t1 * y2) * r;
		float tz = (t2 * z1 - t1 * z2) * r;

		// Adjust tangents of each vert of the triangle
		vertList[3 * i + 0].tangent[0] += tx;
		vertList[3 * i + 0].tangent[1] += ty;
		vertList[3 * i + 0].tangent[2] += tz;

		vertList[3 * i + 1].tangent[0] += tx;
		vertList[3 * i + 1].tangent[1] += ty;
		vertList[3 * i + 1].tangent[2] += tz;

		vertList[3 * i + 2].tangent[0] += tx;
		vertList[3 * i + 2].tangent[1] += ty;
		vertList[3 * i + 2].tangent[2] += tz;


		for (int j = 0; j < 3; j++)
		{
			// Grab the two vectors
			glm::vec3 normal = glm::vec3(
				vertList[3 * i + j].normal[0],
				vertList[3 * i + j].normal[1],
				vertList[3 * i + j].normal[2]
			);

			glm::vec3 tangent = glm::vec3(
				vertList[3 * i + j].tangent[0],
				vertList[3 * i + j].tangent[1],
				vertList[3 * i + j].tangent[2]
			);

			// Use Gram-Schmidt orthogonalize
			tangent = glm::normalize(
				tangent - normal * glm::dot(normal, tangent));

			vertList[3 * i + j].tangent[0] = tangent[0];
			vertList[3 * i + j].tangent[1] = tangent[1];
			vertList[3 * i + j].tangent[2] = tangent[2];
		}
	}

	// close the file that we opened
	fclose(f);

	// Part 6
	// Build our buffers

	// Create empty creationInfo
	// we will be re-using this several times
	// Leave it empty for now
	VkBufferCreateInfo info = {};

	// Vertex Buffer
	//=====================================

	// The size of our Vertex Array, will be the amount of 
	// elements (36) multiplied by the size of one vertex
	uint32_t vertexArraySize = (uint32_t)vertices.size() * sizeof(VertexTangent);

	// Lets make a buffer that is designed to be
	// a source (TRANSFER_SRC), that is large enough
	// to hold our vertex buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = vertexArraySize;

	// make the buffer, and store the data into the buffer
	// For more information on how this works, look at BufferCPU.cpp
	// Learning about BufferCPU is optional
	vertexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	vertexDataCPU->Store(vertList, vertexArraySize);

	// This next buffer will be on the GPU, it is designed to be a 
	// destination (TRANSFER_DST) for our data, and it will also be
	// a VERTEX_BUFFER, which allows us to bind this buffer as a
	// vertex buffer when we set up the pipeline
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = vertexArraySize;

	// build the buffer, and make a command to send the data from the CPU 
	// buffer to the GPU buffer. This command will be added to the initCmd
	// command buffer, and the data will finally be copied from CPU to GPU
	// when we execute the initCmd command buffer (later in prepare).
	// After that, we will delete the CPU buffer, becasue it won't be
	// needed once the data is copied to GPU.
	// For more information on how this works, look at BufferGPU.cpp
	// Learning about BufferGPU is optional
	vertexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	vertexDataGPU->Store(demo->initCmd, vertexDataCPU, vertexArraySize);

	// Index Buffer
	//=====================================

	// The size of this index array will be the number
	// of elements, multiplied by the size of one element
	uint32_t indexArraySize = (int)indices.size() * sizeof(uint16_t);

	// This is the Index CPU buffer, so we useTRANSFER_SRC 
	// because we will copy the buffer to GPU
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = indexArraySize;

	// Just like before, we make the buffer and we store data into it
	indexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	indexDataCPU->Store(indices.data(), indexArraySize);

	// This is the Index GPU buffer, so we use TRANSFER_DST 
	// because we will get data from CPU, just like the Vertex
	// GPU buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = indexArraySize;

	// build the buffer, and add a command to the initCmd command buffer
	// to copy data from the CPU buffer to the GPU buffer
	indexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	indexDataGPU->Store(demo->initCmd, indexDataCPU, indexArraySize);

	// This will be saved to the demo class
	numIndices = (int)indices.size();

	// initialize min and max
	for (int i = 0; i < 3; i++)
	{
		min[i] = vertList[0].position[i];
		max[i] = vertList[0].position[i];
	}

	// get min and max for collision
	for (int i = 1; i < (int)vertices.size(); i++)
	{
		// initialize min and max
		for (int j = 0; j < 3; j++)
		{
			if (vertList[i].position[j] < min[j]) min[j] = vertList[i].position[j];
			if (vertList[i].position[j] > max[j]) max[j] = vertList[i].position[j];
		}
	}

	// get halfwidth/center for collision
	for (int i = 0; i < 3; i++) {
		halfwidth[i] = (max[i] - min[i]) / 2.0f;
		center[i] = (max[i] + min[i]) / 2.0f;
	}
}

Mesh::Mesh(char* file, bool tangents)
{
	// There is probably some way to 
	// make this more optimized, but
	// for now, at least it works

	if (!tangents)
	{
		LoadBasic(file);
	}

	else
	{
		LoadTangents(file);
	}
}

Mesh::Mesh(VertexColor* vertList, int numVerts)
{
	Demo* demo = Demo::GetInstance();

	// make our index buffer based on number
	// of vertices in the mesh
	std::vector<uint16_t> indices;

	// index buffer is 0, 1, 2...
	for (int i = 0; i < numVerts; i++)
		indices.push_back(i);

	// Create empty creationInfo
	// we will be re-using this several times
	// Leave it empty for now
	VkBufferCreateInfo info = {};

	// Vertex Buffer
	//=====================================

	// The size of our Vertex Array, will be the amount of 
	// elements (36) multiplied by the size of one vertex
	uint32_t vertexArraySize = (uint32_t)numVerts * sizeof(VertexColor);

	// Lets make a buffer that is designed to be
	// a source (TRANSFER_SRC), that is large enough
	// to hold our vertex buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = vertexArraySize;

	// make the buffer, and store the data into the buffer
	// For more information on how this works, look at BufferCPU.cpp
	// Learning about BufferCPU is optional
	vertexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	vertexDataCPU->Store(vertList, vertexArraySize);

	// This next buffer will be on the GPU, it is designed to be a 
	// destination (TRANSFER_DST) for our data, and it will also be
	// a VERTEX_BUFFER, which allows us to bind this buffer as a
	// vertex buffer when we set up the pipeline
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = vertexArraySize;

	// build the buffer, and make a command to send the data from the CPU 
	// buffer to the GPU buffer. This command will be added to the initCmd
	// command buffer, and the data will finally be copied from CPU to GPU
	// when we execute the initCmd command buffer (later in prepare).
	// After that, we will delete the CPU buffer, becasue it won't be
	// needed once the data is copied to GPU.
	// For more information on how this works, look at BufferGPU.cpp
	// Learning about BufferGPU is optional
	vertexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	vertexDataGPU->Store(demo->initCmd, vertexDataCPU, vertexArraySize);

	// Index Buffer
	//=====================================

	// The size of this index array will be the number
	// of elements, multiplied by the size of one element
	uint32_t indexArraySize = (int)indices.size() * sizeof(uint16_t);

	// This is the Index CPU buffer, so we useTRANSFER_SRC 
	// because we will copy the buffer to GPU
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = indexArraySize;

	// Just like before, we make the buffer and we store data into it
	indexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	indexDataCPU->Store(indices.data(), indexArraySize);

	// This is the Index GPU buffer, so we use TRANSFER_DST 
	// because we will get data from CPU, just like the Vertex
	// GPU buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = indexArraySize;

	// build the buffer, and add a command to the initCmd command buffer
	// to copy data from the CPU buffer to the GPU buffer
	indexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	indexDataGPU->Store(demo->initCmd, indexDataCPU, indexArraySize);

	// This will be saved to the demo class
	numIndices = (int)indices.size();

	// initialize min and max
	for (int i = 0; i < 3; i++)
	{
		min[i] = vertList[0].position[i];
		max[i] = vertList[0].position[i];
	}

	// get min and max for collision
	for (int i = 1; i < (int)numVerts; i++)
	{
		// initialize min and max
		for (int j = 0; j < 3; j++)
		{
			if (vertList[i].position[j] < min[j]) min[j] = vertList[i].position[j];
			if (vertList[i].position[j] > max[j]) max[j] = vertList[i].position[j];
		}
	}

	// get halfwidth/center for collision
	for (int i = 0; i < 3; i++) {
		halfwidth[i] = (max[i] - min[i]) / 2.0f;
		center[i] = (max[i] + min[i]) / 2.0f;
	}
		
}

Mesh::Mesh(VertexWire* vertList, int numVerts)
{
	Demo* demo = Demo::GetInstance();

	// make our index buffer based on number
	// of vertices in the mesh
	std::vector<uint16_t> indices;

	// index buffer is 0, 1, 2...
	for (int i = 0; i < numVerts; i++)
		indices.push_back(i);

	// Create empty creationInfo
	// we will be re-using this several times
	// Leave it empty for now
	VkBufferCreateInfo info = {};

	// Vertex Buffer
	//=====================================

	// The size of our Vertex Array, will be the amount of 
	// elements (36) multiplied by the size of one vertex
	uint32_t vertexArraySize = (uint32_t)numVerts * sizeof(VertexWire);

	// Lets make a buffer that is designed to be
	// a source (TRANSFER_SRC), that is large enough
	// to hold our vertex buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = vertexArraySize;

	// make the buffer, and store the data into the buffer
	// For more information on how this works, look at BufferCPU.cpp
	// Learning about BufferCPU is optional
	vertexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	vertexDataCPU->Store(vertList, vertexArraySize);

	// This next buffer will be on the GPU, it is designed to be a 
	// destination (TRANSFER_DST) for our data, and it will also be
	// a VERTEX_BUFFER, which allows us to bind this buffer as a
	// vertex buffer when we set up the pipeline
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = vertexArraySize;

	// build the buffer, and make a command to send the data from the CPU 
	// buffer to the GPU buffer. This command will be added to the initCmd
	// command buffer, and the data will finally be copied from CPU to GPU
	// when we execute the initCmd command buffer (later in prepare).
	// After that, we will delete the CPU buffer, becasue it won't be
	// needed once the data is copied to GPU.
	// For more information on how this works, look at BufferGPU.cpp
	// Learning about BufferGPU is optional
	vertexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	vertexDataGPU->Store(demo->initCmd, vertexDataCPU, vertexArraySize);

	// Index Buffer
	//=====================================

	// The size of this index array will be the number
	// of elements, multiplied by the size of one element
	uint32_t indexArraySize = (int)indices.size() * sizeof(uint16_t);

	// This is the Index CPU buffer, so we useTRANSFER_SRC 
	// because we will copy the buffer to GPU
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	info.size = indexArraySize;

	// Just like before, we make the buffer and we store data into it
	indexDataCPU = new BufferCPU(demo->device, demo->memory_properties, info);
	indexDataCPU->Store(indices.data(), indexArraySize);

	// This is the Index GPU buffer, so we use TRANSFER_DST 
	// because we will get data from CPU, just like the Vertex
	// GPU buffer
	info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	info.usage = VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	info.size = indexArraySize;

	// build the buffer, and add a command to the initCmd command buffer
	// to copy data from the CPU buffer to the GPU buffer
	indexDataGPU = new BufferGPU(demo->device, demo->memory_properties, info);
	indexDataGPU->Store(demo->initCmd, indexDataCPU, indexArraySize);

	// This will be saved to the demo class
	numIndices = (int)indices.size();

	// initialize min and max
	for (int i = 0; i < 3; i++)
	{
		min[i] = vertList[0].position[i];
		max[i] = vertList[0].position[i];
	}

	// get min and max for collision
	for (int i = 1; i < (int)numVerts; i++)
	{
		// initialize min and max
		for (int j = 0; j < 3; j++)
		{
			if (vertList[i].position[j] < min[j]) min[j] = vertList[i].position[j];
			if (vertList[i].position[j] > max[j]) max[j] = vertList[i].position[j];
		}
	}

	// get halfwidth/center for collision
	for (int i = 0; i < 3; i++) {
		halfwidth[i] = (max[i] - min[i]) / 2.0f;
		center[i] = (max[i] + min[i]) / 2.0f;
	}
}

Mesh::~Mesh()
{
	delete vertexDataGPU;
	delete indexDataGPU;
}

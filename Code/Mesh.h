#pragma once
#include "BufferCPU.h"
#include "BufferGPU.h"
#include "VertexColor.h"
#include "VertexWire.h"

class Mesh
{
public:
	float min[3];
	float max[3];
	float center[3];
	float halfwidth[3];

	// Mesh data
	BufferCPU* vertexDataCPU;
	BufferGPU* vertexDataGPU;
	BufferCPU* indexDataCPU;
	BufferGPU* indexDataGPU;

	Mesh(VertexColor* vertList, int numVerts);
	Mesh(VertexWire* vertList, int numVerts);
	Mesh(char* file, bool tangents);
	~Mesh();

	int numIndices;
	void LoadBasic(char* file);
	void LoadTangents(char* file);
};


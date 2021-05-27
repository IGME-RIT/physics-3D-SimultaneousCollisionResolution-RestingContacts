#include "Cuboid.h"

// Possible issue that it does draw the inside of the rectangle, which is inefficient.
Cuboid::Cuboid(glm::vec3 center, glm::vec3 halfwidth, glm::vec3 color)
{
	this->center = center;
	this->halfwidth = halfwidth;
	this->min = center - halfwidth;
	this->max = center + halfwidth;

	// Need 36 points to create the cuboid (12 triangles, 3 points each).
	VertexColor* vertexList = new VertexColor[36];

	// Assign all of the colors first.
	for (int i = 0; i < 36; i++) {
		for (int j = 0; j < 3; j++) {
			vertexList[i].color[j] = color[j];
		}
	}

	// Lambda function to speed up assignment to vertex list.
	auto assignToVertex = [=](float* position, bool m0, bool m1, bool m2) {
		position[0] = (m0 ? max[0] : min[0]);
		position[1] = (m1 ? max[1] : min[1]);
		position[2] = (m2 ? max[2] : min[2]);
	};

	// Min X face.
	assignToVertex(vertexList[0].position, 0, 0, 0);
	assignToVertex(vertexList[1].position, 0, 0, 1);
	assignToVertex(vertexList[2].position, 0, 1, 1);

	assignToVertex(vertexList[3].position, 0, 0, 0);
	assignToVertex(vertexList[4].position, 0, 1, 1);
	assignToVertex(vertexList[5].position, 0, 1, 0);

	// Min Y face.
	assignToVertex(vertexList[6].position, 0, 0, 0);
	assignToVertex(vertexList[7].position, 1, 0, 0);
	assignToVertex(vertexList[8].position, 1, 0, 1);

	assignToVertex(vertexList[9].position, 0, 0, 0);
	assignToVertex(vertexList[10].position, 1, 0, 1);
	assignToVertex(vertexList[11].position, 0, 0, 1);

	// Min Z face.
	assignToVertex(vertexList[12].position, 0, 0, 0);
	assignToVertex(vertexList[13].position, 0, 1, 0);
	assignToVertex(vertexList[14].position, 1, 1, 0);

	assignToVertex(vertexList[15].position, 0, 0, 0);
	assignToVertex(vertexList[16].position, 1, 1, 0);
	assignToVertex(vertexList[17].position, 1, 0, 0);

	// Max X face.
	assignToVertex(vertexList[18].position, 1, 1, 1);
	assignToVertex(vertexList[19].position, 1, 1, 0);
	assignToVertex(vertexList[20].position, 1, 0, 0);

	assignToVertex(vertexList[21].position, 1, 1, 1);
	assignToVertex(vertexList[22].position, 1, 0, 0);
	assignToVertex(vertexList[23].position, 1, 0, 1);

	// Min Y face.
	assignToVertex(vertexList[24].position, 1, 1, 1);
	assignToVertex(vertexList[25].position, 0, 1, 1);
	assignToVertex(vertexList[26].position, 0, 1, 0);

	assignToVertex(vertexList[27].position, 1, 1, 1);
	assignToVertex(vertexList[28].position, 0, 1, 0);
	assignToVertex(vertexList[29].position, 1, 1, 0);

	// Min Z face.
	assignToVertex(vertexList[30].position, 1, 1, 1);
	assignToVertex(vertexList[31].position, 1, 0, 1);
	assignToVertex(vertexList[32].position, 0, 0, 1);

	assignToVertex(vertexList[33].position, 1, 1, 1);
	assignToVertex(vertexList[34].position, 0, 0, 1);
	assignToVertex(vertexList[35].position, 0, 1, 1);

	// Generate the mesh based on the input and created vertex color list.
	mesh = std::make_shared<Mesh>(vertexList, 36);

	// do NOT delete colorList, it will
	// be deleted automatically after it
	// is passed to the GPU, by our smart
	// BufferCPU / BufferGPU system

	// create an entity for the mesh
	entity = std::make_shared<Entity>();
	entity->mesh = mesh.get();
	entity->CreateDescriptorSetColor();

}

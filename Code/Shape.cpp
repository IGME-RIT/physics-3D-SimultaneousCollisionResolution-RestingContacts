#include "Shape.h"

#include <glm/gtc/type_ptr.hpp>

// Getters and Setters
const glm::vec3 Shape::GetPosition() { return position; }
void Shape::SetPosition(const glm::vec3 position) { this->position = position; }


// Constructor for shapes.
// Points contains pairs positions and colors. First point will represent the origin in shape's local space.
Shape::Shape(std::vector<glm::vec3> points)
{
	// Make sure there is an even number of points, greater than one.
	assert(points.size() >= 2);
	assert(points.size() % 2 == 0);

	// Calculate the total number of triangles we'll have to store.
	int numOfTriangles = (points.size() / 2) - 2;

	// List of points to pass into the entity.
	VertexColor* colorList = new VertexColor[numOfTriangles * 3];

	// Set initial position to the first point.
	position = points[0];

	// Create triangles based on input points, and assign colors appropriately.
	for (int i = 1; i <= numOfTriangles; i++) {
		// points to use for pos: 0, (i*2), (i*2)+2
		// points to use for color: 1, (i*2)+1, (i*2)+3
		int colorIndex = (i - 1) * 3;
		for (int j = 0; j < 3; j++) {
			
			colorList[colorIndex].position[j] = points[0][j];
			colorList[colorIndex].color[j] = points[1][j];

			colorList[colorIndex + 1].position[j] = points[(i * 2)][j];
			colorList[colorIndex + 1].color[j] = points[(i * 2) + 1][j];

			colorList[colorIndex + 2].position[j] = points[(i * 2) + 2][j];
			colorList[colorIndex + 2].color[j] = points[(i * 2) + 3][j];
		}
		
	}

	// Generate the mesh based on the input and created vertex color list.
	mesh = std::make_shared<Mesh>(colorList, numOfTriangles * 3);

	// do NOT delete colorList, it will
	// be deleted automatically after it
	// is passed to the GPU, by our smart
	// BufferCPU / BufferGPU system

	// create an entity for the mesh
	entity = std::make_shared<Entity>();
	entity->mesh = mesh.get();
	entity->CreateDescriptorSetColor();
}

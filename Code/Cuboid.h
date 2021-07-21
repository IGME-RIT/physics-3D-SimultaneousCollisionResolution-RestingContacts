#pragma once

// Cuboid class is a helper class that makes the creation of Cuboid entities a lot easier.
// Constructor takes in the center point, halfwidth, and an optional color. This creates
// two entities, one that is the actual cuboid, and one that is a wireframe of the cuboid.
// This makes it easier to see, as there's no shadows in the scene.
// 
// Written by Chris Hambacher, 2021.

#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"
#include "Entity.h"
#include <memory>

// Refactor the member variables to be private and use getters.
class Cuboid
{
private:
public:
	Cuboid(glm::vec3 center, glm::vec3 halfwidth, glm::vec3 color = glm::vec3(1, 1, 1));
	void Draw(Demo* d);

	glm::vec3 halfwidth;
	glm::vec3 center;
	glm::vec3 min;
	glm::vec3 max;
	
	std::vector<std::shared_ptr<Entity>> GetEntityPointers() const;

	std::shared_ptr<Entity> solidEntity;
	std::shared_ptr<Entity> wireEntity;
};


#pragma once

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


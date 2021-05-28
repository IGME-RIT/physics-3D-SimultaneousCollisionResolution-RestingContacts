#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"
#include "Entity.h"
#include <memory>

class Cuboid
{
private:
public:
	Cuboid(glm::vec3 center, glm::vec3 halfwidth, glm::vec3 color = glm::vec3(1, 1, 1));
	
	glm::vec3 halfwidth;
	glm::vec3 center;
	glm::vec3 min;
	glm::vec3 max;
	
	std::shared_ptr<Entity> entity;
};


#pragma once

#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"
#include "Entity.h"
#include <memory>



// Superclass for any 2d shapes.
class Shape
{
private:
	glm::vec3 position;
public:

	Shape(std::vector<glm::vec3> points);

	inline const glm::vec3 GetPosition();
	void SetPosition(const glm::vec3 position);
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Entity> entity;
};


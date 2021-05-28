#pragma once


#include <glm/glm.hpp>
#include <vector>
#include "Mesh.h"
#include "Entity.h"
#include <memory>



// A class to represent a physics object.
class Rigidbody
{
private:

	std::shared_ptr<Entity> entity;

	// Physics related attributes (double precision).
	glm::dvec3 a = glm::dvec3(0);	// Acceleration
	glm::dvec3 v = glm::dvec3(0);	// Velocity
	glm::dvec3 p = glm::dvec3(0); // Position

	float gravity = -1.0f;

	// Flags for this object (can create bitwise flags if enough show up)
	bool isMovable;

	// Mesh related attributes (held in this class as vec3 instead of float arrays).
	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 center;
	glm::vec3 halfwidth;

public:
	// Create a rigidbody from the given entity (if using cuboid, just pass entity).
	Rigidbody(std::shared_ptr<Entity> entity, bool isMovable = true);

	void Update(double dt);

	std::shared_ptr<Entity> GetEntity() const;
	const glm::vec3& GetMin() const;
	const glm::vec3& GetMax() const;
	const glm::vec3& GetCenter() const;
	const glm::vec3& GetHalfwidth() const;
};


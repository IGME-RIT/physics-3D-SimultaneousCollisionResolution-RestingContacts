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

public:
	// Create a rigidbody from the given entity (if using cuboid, just pass entity).
	Rigidbody(std::shared_ptr<Entity> entity, bool isMovable = true);

	void Update(double dt);

};


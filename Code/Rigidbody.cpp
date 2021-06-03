

#include "Rigidbody.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // Used for glm::make_vec3, which converts from float* to glm::vec3.

// Constructor delegation.
Rigidbody::Rigidbody(std::shared_ptr<Entity> entity, bool isMovable) : Rigidbody::Rigidbody(std::vector<std::shared_ptr<Entity>>{entity}, isMovable) {}

Rigidbody::Rigidbody(std::vector<std::shared_ptr<Entity>> entities, bool isMovable)
{
	assert(entities.size() > 0);

	this->entities = entities;
	this->isMovable = isMovable;
	// We assume that the first entity is the primary.
	this->entity = entities[0];

	this->p = this->entity->pos;

	// Assign all of the mesh variables (we use the things from the first entity)..
	const Mesh& mesh = *(entity->mesh);
	this->min = glm::make_vec3(mesh.min);
	this->max = glm::make_vec3(mesh.max);
	this->center = glm::make_vec3(mesh.center);
	this->halfwidth = glm::make_vec3(mesh.halfwidth);
}

void Rigidbody::Update(double h)
{

	/*  
	 *  Movement Section
 	 */
	if (!isMovable) return;
	a = glm::dvec3(0, gravity, 0);

	// Apply gravity
	glm::dvec3 v_next = v + a * h;
	glm::dvec3 p_next = p + v * h;


	// Apply the position to the entity.
	for (std::shared_ptr<Entity> entity : entities) {
		entity->pos = p_next;
	}
	

	// Replace past values with new values.
	v = v_next;
	p = p_next;
}

glm::vec3 Rigidbody::GetAxis(unsigned best) const {
	// We take modulo 3 to account for second set of axis (so 3 should map to 0, 4 should map to 1, etc.)
	return static_cast<glm::vec3>(entity->GetModelMatrix()[best % 3]);
}
const glm::mat4& Rigidbody::GetModelMatrix() const
{
	return entity->GetModelMatrix();
}
;

std::shared_ptr<Entity> Rigidbody::GetEntity() const { return entity; }
unsigned Rigidbody::GetEntityCount() const { return entities.size(); }

const glm::vec3& Rigidbody::GetMin() const { return min; }
const glm::vec3& Rigidbody::GetMax() const { return max; }
const glm::vec3& Rigidbody::GetCenter() const { return center; }
const glm::vec3& Rigidbody::GetHalfwidth() const { return halfwidth; }

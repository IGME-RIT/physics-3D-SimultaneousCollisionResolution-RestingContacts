

#include "Rigidbody.h"
#include <iostream>

Rigidbody::Rigidbody(std::shared_ptr<Entity> entity, bool isMovable)
{
	assert(entity->mesh != nullptr);

	this->entity = entity;

	std::cout << "Number of count: " << this->entity.use_count() << std::endl;

	this->isMovable = isMovable;

	this->p = this->entity->pos;
}

void Rigidbody::Update(double h)
{
	a = glm::dvec3(0, gravity, 0);

	// Apply gravity
	glm::dvec3 v_next = v + a * h;
	glm::dvec3 p_next = p + v * h;

	// Apply the position to the entity.
	entity->pos = p_next;

	// Replace past values with new values.
	v = v_next;
	p = p_next;
}

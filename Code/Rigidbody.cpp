

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

	// Assign all of the mesh variables (we use the things from the first entity).
	const Mesh& mesh = *(entity->mesh);
	this->min = glm::make_vec3(mesh.min);
	this->max = glm::make_vec3(mesh.max);
	this->center = glm::make_vec3(mesh.center);
	this->halfwidth = glm::make_vec3(mesh.halfwidth);
	this->radius = glm::length(halfwidth);

	// Assume that we dealing with cuboids.
	this->mass = 1;
	this->momentOfInertia = glm::mat3(
		mass * (glm::pow(halfwidth.y * 2.f, 2) + glm::pow(halfwidth.z * 2.f, 2)) / 12.f, 0, 0,
		0, mass * (glm::pow(halfwidth.x * 2.f, 2) + glm::pow(halfwidth.z * 2.f, 2)) / 12.f, 0,
		0, 0, mass * (glm::pow(halfwidth.x * 2.f, 2) + glm::pow(halfwidth.y * 2.f, 2)) / 12.f
	);
	this->inverseMass = 1 / mass;
	this->inverseInertia = glm::inverse(momentOfInertia);	// This can be optimized.

	// Assign the state.
	//this->currentState = Rigidbody::State(this->entity->pos, glm::quat(), glm::vec3(0), glm::vec3(0));
	this->currentState = Rigidbody::State(this->entity->pos, glm::quat(), glm::vec3(0), glm::vec3(0));
	this->computedState = this->currentState;
	this->newState = this->currentState;

	//AddConstantForce(glm::vec3(0, gravity, 0));
}

void Rigidbody::Update(double h)
{

	/*  
	 *  Movement Section
 	 */
	if (!isMovable) return;

	// Compute the next state.
	computedState = currentState.ComputeRigidDerivative(mass, inverseInertia, forces, constantForces);
	newState = currentState + computedState * h;
	newState.orientation = glm::normalize(newState.orientation);

	// Apply the position to the entity.
	for (std::shared_ptr<Entity> entity : entities) {
		entity->rotQuat = newState.orientation;
		entity->pos = newState.pos;
	}
	
	// Replace past values with new values.
	currentState = newState;

	// Clear the forces vector.
	forces.clear();
}

void Rigidbody::AddForce(glm::vec3 forceVector)
{
	Force force;
	force.force = forceVector;
	forces.push_back(std::move(force));
}

void Rigidbody::AddForce(glm::vec3 forceVector, glm::vec3 position)
{
	Force force;
	force.force = forceVector;
	force.position = position;
	force.hasPosition = true;
	forces.push_back(std::move(force));
}

void Rigidbody::AddConstantForce(glm::vec3 forceVector)
{
	Force force;
	force.force = forceVector;
	constantForces.push_back(std::move(force));
}

Rigidbody::State Rigidbody::State::ComputeRigidDerivative(
	float mass, 
	glm::mat3 inverseMomentOfIntertia, 
	std::vector<Force> forces, 
	std::vector<Force> constantForces
)
{
	State S;	// State to return.
	S.pos = this->momentum / mass;

	// Calculate new orientation.
	glm::mat3 R = glm::toMat3(orientation);
	glm::mat3 worldI = R * inverseMomentOfIntertia * glm::transpose(R);
	glm::vec3 omega = worldI * angularMomentum;
	glm::quat omega_p = glm::quat(0, omega);
	S.orientation = 0.5f * omega_p * this->orientation;

	// Calculate forces.
	S.momentum = S.angularMomentum = glm::vec3(0);
	// Apply impulses.
	for (Force force : forces) {
		S.momentum += force.force;
		// If the force has a position, it affects torque.
		if (force.hasPosition == true) {
			glm::vec3 r = force.position - S.pos;
			S.angularMomentum += glm::cross(r, force.force);
		}
	}
	// Apply constant forces.
	for (Force force : constantForces) {
		S.momentum += force.force;
	}

	return S;
}




#pragma region Getters and Setters
glm::vec3 Rigidbody::GetAxis(unsigned best) const {
	return static_cast<glm::vec3>(entity->GetModelMatrix()[best]);
}
const glm::mat4& Rigidbody::GetModelMatrix() const { return entity->GetModelMatrix(); }
std::shared_ptr<Entity> Rigidbody::GetEntity() const { return entity; }
unsigned Rigidbody::GetEntityCount() const { return entities.size(); }

const glm::vec3& Rigidbody::GetMin() const { return min; }
const glm::vec3& Rigidbody::GetMax() const { return max; }
const glm::vec3& Rigidbody::GetCenter() const { return center; }
const glm::vec3& Rigidbody::GetHalfwidth() const { return halfwidth; }
const float& Rigidbody::GetRadius() const { return radius; }
const float& Rigidbody::GetInverseMass() const { return inverseMass; }
const glm::mat3& Rigidbody::GetInverseIntertia() const { return inverseInertia; }
const glm::vec3& Rigidbody::GetVelocity() const { return currentState.momentum * inverseMass; }
const glm::vec3& Rigidbody::GetAngularVelocity() const { return inverseInertia * currentState.angularMomentum; }
const glm::vec3& Rigidbody::GetPosition() const { return entity->GetWorldPosition(); }
const glm::vec3& Rigidbody::GetAngularMomentum() const { return currentState.angularMomentum; }
Rigidbody::State& Rigidbody::GetState()
{
	return currentState;
}
const glm::vec3 Rigidbody::GetForce() const { 
	glm::vec3 totalForce = glm::vec3(0);
	for (auto force : forces)
		totalForce += force;
	for (auto force : constantForces)
		totalForce += force;
	return totalForce;
}
const glm::vec3 Rigidbody::GetTorque() const {
	glm::vec3 totalTorque = glm::vec3(0);
	for (Force force : forces) {
		if (force.hasPosition == true) {
			glm::vec3 r = force.position - currentState.pos;
			totalTorque += glm::cross(r, force.force);
		}
	}
	return totalTorque;
}


#pragma endregion Getters and Setters


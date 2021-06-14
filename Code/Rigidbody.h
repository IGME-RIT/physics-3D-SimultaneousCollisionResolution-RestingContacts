#pragma once


#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include "Mesh.h"
#include "Entity.h"
#include <memory>
#include "Collisions.h"



// A class to represent a physics object.
class Rigidbody
{
public:

	// A struct to represent a force (as a force can be a direction, or position and direction).
	struct Force {
		glm::vec3 force = glm::vec3(0);
		glm::vec3 position = glm::vec3(0);
		bool hasPosition = false;
	};

	// A struct that contains all the necessary physics information about the state of the rigidbody.
	// Idea from the book "Foundations of Physically Based Modeling and Animation", page 201.
	struct State {
		glm::vec3 pos;				// x
		glm::quat orientation;		// q, Quaternion
		glm::vec3 momentum;			// P
		glm::vec3 angularMomentum;	// L

		
		State(glm::vec3 pos, glm::quat orientation, glm::vec3 momentum, glm::vec3 angMomentum) {
			this->pos = pos;
			this->orientation = orientation;
			this->momentum = momentum;
			this->angularMomentum = angMomentum;
		}
		State() : State(glm::vec3(0), glm::quat(), glm::vec3(0), glm::vec3(0)) {}
		State operator+ (const State& first) const { return State(pos + first.pos, orientation + first.orientation, momentum + first.momentum, angularMomentum + first.angularMomentum); }
		State operator* (const float& scalar) const { return State(scalar * pos, scalar * orientation, scalar * momentum, scalar * angularMomentum); }

		// Main function of the struct, inverse moment of inertia is in local space.
		State ComputeRigidDerivative(float mass,
			glm::mat3 inverseMomentOfIntertia,
			std::vector<Rigidbody::Force> forces,
			std::vector<Rigidbody::Force> constantForces);
	};

private:

	std::vector<std::shared_ptr<Entity>> entities;
	std::shared_ptr<Entity> entity;	// First entity in the array.

	// Physics related attributes (double precision).
	// Physics variables: m, x, v, P,  I, r, w, L
	// The state of the rigidbody.
	State currentState;
	float mass;
	float inverseMass;
	glm::mat3 momentOfInertia;
	glm::mat3 inverseInertia;
	std::vector<Force> forces;	// List of forces currently being applied to the object (cleared every frame). Impulses.
	std::vector<Force> constantForces; // List of forces that are applied to object every frame (like gravity). Not cleared at the end of the frame.

	float gravity = -1.0f;

	// Flags for this object (can create bitwise flags if enough show up)
	bool isMovable = true;

	// Mesh related attributes (held in this class as vec3 instead of float arrays).
	glm::vec3 min;
	glm::vec3 max;
	glm::vec3 center;
	glm::vec3 halfwidth;
	float radius;	// Calculated from halfwidth.

public:
	// Constructor to initialize one rigidbody for multiple entities (the entities will all move the same).
	// Used in cases such as having an object and its OBB moving at the same time.
	Rigidbody(std::vector<std::shared_ptr<Entity>> entities, bool isMovable = true);

	// Create a rigidbody from the given entity (if using cuboid, just pass entity).
	Rigidbody(std::shared_ptr<Entity> entity, bool isMovable = true);

	// Update the physics.
	void Update(double dt);

	// Add a force to the rigidbody.
	void AddForce(glm::vec3 forceVector);
	void AddForce(glm::vec3 forceVector, glm::vec3 position);
	void AddConstantForce(glm::vec3 forceVector);	// I don't think there's any constant point forces?

	// Get the axis represented by the given number (0 <= best <= 2).
	glm::vec3 GetAxis(unsigned best) const;

	// Getters and setters.
	const glm::mat4& GetModelMatrix() const;
	std::shared_ptr<Entity> GetEntity() const;
	unsigned GetEntityCount() const;
	const glm::vec3& GetMin() const;
	const glm::vec3& GetMax() const;
	const glm::vec3& GetCenter() const;
	const glm::vec3& GetHalfwidth() const;
	const float& GetRadius() const;
	const float& GetInverseMass() const;
	const glm::mat3& GetInverseIntertia() const;
	const glm::vec3& GetVelocity() const;
	const glm::vec3& GetAngularVelocity() const;
	const glm::vec3& GetPosition() const;
	const glm::vec3 GetForce() const;	// Get the total forces acting on this body.
	const glm::vec3 GetTorque() const;
	const glm::vec3& GetAngularMomentum() const;

	State& GetState();


private:
	// Other variables (precreate for optimization purposes)
	State newState;
	State computedState;
};


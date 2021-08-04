#pragma once

// The rigidbody class is used to represent a physics object. Logic mostly from 
// David Eberly's book, "Game Physics, 2nd edition". Each rigidbody can store any
// amount of entities; the first entity will be considered the primary, and all
// the secondary entities will be moved the same as the primary.
//
// It's important to note that many of the rigidbody functions are hardcoded to work
// with cuboids, but they shouldn't be too hard to convert to work with hulls.
//
// Written by Chris Hambacher, 2021.

#include <glm/glm.hpp>
#include <glm/gtx/quaternion.hpp>
#include <vector>
#include "Mesh.h"
#include "Entity.h"
#include <memory>
//#include "Collisions.h"




class Rigidbody
{
protected:
	// Typedef used for force/torque equations. Contains the state of object/current time.
	typedef glm::vec3(*Function)
	(
		double,		// current time (not dt)
		glm::vec3,	// position
		glm::quat,	// orientation
		glm::vec3,	// momentum
		glm::vec3,	// angular momentum
		glm::mat3,	// orientation matrix
		glm::vec3,	// velocity
		glm::vec3,	// angular velocity
		float		// mass
	);

public:

	// Constructor to initialize one rigidbody for multiple entities (the entities will all move the same).
	// Used in cases such as having an object and its OBB moving at the same time.
	Rigidbody(std::vector<std::shared_ptr<Entity>> entities, bool isMovable = true, float mass = 1.0f);

	// Create a rigidbody from the given entity and mass.
	Rigidbody(std::shared_ptr<Entity> entity, bool isMovable = true, float mass = 1.0f);

	// State based functions.
	void SetState(glm::vec3 position, glm::quat orientation, glm::vec3 momentum, glm::vec3 angularMomentum);
	void GetState(glm::vec3& position, glm::quat& orientation, glm::vec3& momentum, glm::vec3& angularMomentum) const;
	void GetPosition(glm::vec3& position) const;	// Used enough to have it's own function.

	// Set the force/torque functions to use in update.
	void SetForceFunction(Function force);
	void SetTorqueFunction(Function torque);

	void AppendInternalForce(glm::vec3 intForce);
	void AppendInternalTorque(glm::vec3 intTorque);

	// RK4 diff eq solver.
	void Update(float t, float dt);

	// Called from update, updates the values of the entity.
	void Draw();

	// Pulling state out of the struct.
	// State variables.
	glm::vec3 m_position;			// X
	glm::quat m_orientation;		// Q 
	glm::vec3 m_momentum;			// P
	glm::vec3 m_angularMomentum;	// L

	// Derived state variables.
	glm::mat3 m_orientationMatrix;	// R
	glm::vec3 m_velocity;			// V
	glm::vec3 m_angularVelocity;	// W

	// Force variables.
	glm::vec3 m_externalForce, m_externalTorque;	// External forces at time of simulation.
	glm::vec3 m_internalForce, m_internalTorque;	// Resting contact forces. Resets to zero each pass.

	// Const properties of object.
	float m_mass;
	glm::mat3 m_inertia;
	float m_invMass;
	glm::mat3 m_invInertia;

	// We keep track of the next dt to use for this object.
	float m_dt;

	// Flags for this object (can create bitwise flags if enough show up)
	bool m_isMovable = true;

// Section for physics related variables.
protected:

	// Get derived state variables from normal state variables.
	void Convert(glm::quat Q, glm::vec3 P, glm::vec3 L, glm::mat3& R, glm::vec3& V, glm::vec3& W) const;

	// Force and torque functions.
	Function m_force;
	Function m_torque;

	// Body inertia tensors (don't change with rotation, used to calculate actuala inertia tensor).
	glm::mat3 m_bodyInertia;
	glm::mat3 m_bodyInvInertia;

	// Damping variables (1 for no damping, 0 for full damping).
	float m_linearDamping = 0.6f;
	float m_angularDamping = 0.6f;
	void SetDamping(float linear, float angular);

// Section for underlying entities and mesh.
public:
	glm::vec3 GetAxis(int best) const;
	glm::vec3 GetLocalAxis(int best) const;
	// Get the support vector of this hull (cuboid) based on input vector.
	glm::vec3 GetSupport(glm::vec3 v) const;
	const glm::mat4 GetModelMatrix() const;

	// Mesh related attributes.
	glm::vec3 m_min;
	glm::vec3 m_max;
	glm::vec3 m_center;
	glm::vec3 m_halfwidth;
	float m_radius;	// Calculated from halfwidth.

protected:

	std::shared_ptr<Entity> m_entity;					// Primary entity to base variables on.
	std::vector<std::shared_ptr<Entity>> m_entities;	// List of all entities to adjust the same.

};

// Adding some force functions as examples.
struct ForceFunctions 
{
	// Comment for easier explaination of the function inputs.
	// Typedef used for force/torque equations. Contains the state of object/current time.
	//typedef glm::vec3(*Function)
	//(
	//	double,		// current time (not dt)
	//	glm::vec3,	// position
	//	glm::quat,	// orientation
	//	glm::vec3,	// momentum
	//	glm::vec3,	// angular momentum
	//	glm::mat3,	// orientation matrix
	//	glm::vec3,	// velocity
	//	glm::vec3,	// angular velocity
	//	float		// mass
	//);

	static glm::vec3 NoForce(double t, glm::vec3 X, glm::quat Q, glm::vec3 P, glm::vec3 L, glm::mat3 R, glm::vec3 V, glm::vec3 W, float m) {
		return glm::vec3(0, 0, 0);
	}

	static glm::vec3 NoTorque(double t, glm::vec3 X, glm::quat Q, glm::vec3 P, glm::vec3 L, glm::mat3 R, glm::vec3 V, glm::vec3 W, float m) {
		return glm::vec3(0, 0, 0);
	}

	static glm::vec3 Gravity(double t, glm::vec3 X, glm::quat Q, glm::vec3 P, glm::vec3 L, glm::mat3 R, glm::vec3 V, glm::vec3 W, float m) {
		return glm::vec3(0.f, -m, 0.f);
	}

	static glm::vec3 Clockwise(double t, glm::vec3 X, glm::quat Q, glm::vec3 P, glm::vec3 L, glm::mat3 R, glm::vec3 V, glm::vec3 W, float m) {
		return glm::vec3(.01f, 0, 0);
	}
};


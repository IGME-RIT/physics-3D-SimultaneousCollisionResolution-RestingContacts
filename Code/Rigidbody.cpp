

#include "Rigidbody.h"
#include <iostream>
#include <glm/gtc/type_ptr.hpp> // Used for glm::make_vec3, which converts from float* to glm::vec3.

// Constructor delegation.
Rigidbody::Rigidbody(std::shared_ptr<Entity> entity, bool isMovable) : Rigidbody::Rigidbody(std::vector<std::shared_ptr<Entity>>{entity}, isMovable) {}

Rigidbody::Rigidbody(std::vector<std::shared_ptr<Entity>> entities, bool isMovable)
{
	assert(entities.size() > 0);

	m_entities = entities;
	m_isMovable = isMovable;
	// We assume that the first entity is the primary.
	m_entity = entities[0];

	// Assign all of the mesh variables (we use the things from the first entity).
	const Mesh& mesh = *(m_entity->mesh);
	m_min = glm::make_vec3(mesh.min);
	m_max = glm::make_vec3(mesh.max);
	m_center = glm::make_vec3(mesh.center);
	m_halfwidth = glm::make_vec3(mesh.halfwidth);
	m_radius = glm::length(m_halfwidth);

	// Assume that we dealing with cuboids.
	m_mass = 1;
	if (isMovable == false) {
		m_mass = 1000000;
	}
	m_bodyInertia = glm::mat3(
		m_mass * (glm::pow(m_halfwidth.y * 2.f, 2) + glm::pow(m_halfwidth.z * 2.f, 2)) / 12.f, 0, 0,
		0, m_mass * (glm::pow(m_halfwidth.x * 2.f, 2) + glm::pow(m_halfwidth.z * 2.f, 2)) / 12.f, 0,
		0, 0, m_mass * (glm::pow(m_halfwidth.x * 2.f, 2) + glm::pow(m_halfwidth.y * 2.f, 2)) / 12.f
	);
	m_invMass = 1 / m_mass;
	m_bodyInvInertia = glm::inverse(m_bodyInertia);	

	// Update the inertia tensors.
	m_inertia = m_bodyInertia;
	m_invInertia = m_bodyInvInertia;

	// Set default position to entity position.
	m_position = m_entity->GetWorldPosition();

}

void Rigidbody::AppendInternalForce(glm::vec3 intForce) { m_internalForce += intForce; }
void Rigidbody::AppendInternalTorque(glm::vec3 intTorque) { m_internalTorque += intTorque; }

void Rigidbody::Update(float dt, float t) {
	
	// Precompute values needed for RK4.
	float halfdt = 0.5f * dt;
	float sixthdt = dt / 6.0f;
	float tphalfdt = t + halfdt;
	float tpdt = t + dt;

	// New pos, momentum, ang momentum, velocity, ang velocity respectively.
	glm::vec3 XN, PN, LN, VN, WN;	
	// New orientation quat.
	glm::quat QN;
	// New orientation matrix.
	glm::mat3 RN;

	// A1 = G(t, S0), B1 = S0 + (dt/2) * A1
	// Calculate derivatives.
	glm::vec3 A1DXDT = m_velocity;
	glm::quat A1DQDT = 0.5f * glm::quat(0, m_angularVelocity) * m_orientation;	// Quaternion representation of angular vel has no real portion.
	//glm::vec3 A1DPDT = m_force(t, m_position, m_orientation, m_momentum, m_angularMomentum, m_orientationMatrix, m_velocity, m_angularVelocity);
	//glm::vec3 A1DLDT = m_torque(t, m_position, m_orientation, m_momentum, m_angularMomentum, m_orientationMatrix, m_velocity, m_angularVelocity);
	glm::vec3 A1DPDT = m_externalForce + m_internalForce;
	glm::vec3 A1DLDT = m_externalTorque + m_internalTorque;
	m_internalForce = glm::vec3(0);
	m_internalTorque = glm::vec3(0);
	// Calculate the RK4 value for first step.
	XN = m_position + halfdt * A1DXDT;
	QN = m_orientation + halfdt * A1DQDT;
	PN = m_momentum + halfdt * A1DPDT;
	LN = m_angularMomentum + halfdt * A1DLDT;
	Convert(QN, PN, LN, RN, VN, WN);
	
	// A2 = G(t + dt / 2, B1), B2 = S0 + (dt / 2) * A2
	glm::vec3 A2DXDT = VN;
	glm::quat A2DQDT = 0.5f * glm::quat(0, WN) * QN;
	glm::vec3 A2DPDT = m_force(tphalfdt, XN, QN, PN, LN, RN, VN, WN);
	glm::vec3 A2DLDT = m_torque(tphalfdt, XN, QN, PN, LN, RN, VN, WN);
	XN = m_position + halfdt * A2DXDT;
	QN = m_orientation + halfdt * A2DQDT;
	PN = m_momentum + halfdt * A2DPDT;
	LN = m_angularMomentum + halfdt * A2DLDT;
	Convert(QN, PN, LN, RN, VN, WN);

	// A3 = G(t + dt / 2,B2), B3 = S0 + dt * A3
	glm::vec3 A3DXDT = VN;
	glm::quat A3DQDT = 0.5f * glm::quat(0, WN) * QN;
	glm::vec3 A3DPDT = m_force(tphalfdt, XN, QN, PN, LN, RN, VN, WN);
	glm::vec3 A3DLDT = m_torque(tphalfdt, XN, QN, PN, LN, RN, VN, WN);
	XN = m_position + dt * A3DXDT;
	QN = m_orientation + dt * A3DQDT;
	PN = m_momentum + dt * A3DPDT;
	LN = m_angularMomentum + dt * A3DLDT;
	Convert(QN, PN, LN, RN, VN, WN);

	// A4 = G(t + dt,B3), S1 = S0 + (dt / 6) * (A1 + 2 * A2 + 2 * A3 + A4)
	glm::vec3 A4DXDT = VN;
	glm::quat A4DQDT = 0.5f * glm::quat(0, WN) * QN;
	glm::vec3 A4DPDT = m_force(tpdt, XN, QN, PN, LN, RN, VN, WN);
	glm::vec3 A4DLDT = m_torque(tpdt, XN, QN, PN, LN, RN, VN, WN);
	m_position += sixthdt * (A1DXDT + 2.0f * (A2DXDT + A3DXDT) + A4DXDT);
	m_orientation += sixthdt * (A1DQDT + 2.0f * (A2DQDT + A3DQDT) + A4DQDT);
	m_momentum += sixthdt * (A1DPDT + 2.0f * (A2DPDT + A3DPDT) + A4DPDT);
	m_angularMomentum += sixthdt * (A1DLDT + 2.0f * (A2DLDT + A3DLDT) + A4DLDT);
	Convert(m_orientation, m_momentum, m_angularMomentum, m_orientationMatrix, m_velocity, m_angularVelocity);
	// All the state variables should have correct and consistent information.
	//m_orientation = glm::normalize(m_orientation);

	// Update the inertia tensors.
	m_inertia = m_orientationMatrix * m_bodyInertia * glm::transpose(m_orientationMatrix);
	m_invInertia = m_orientationMatrix * m_bodyInvInertia * glm::transpose(m_orientationMatrix);

	// Update external force to correspond to new time t + dt.
	m_externalForce = m_force(tpdt, m_position, m_orientation, m_momentum, m_angularMomentum, m_orientationMatrix, m_velocity, m_angularVelocity);
	m_externalTorque = m_torque(tpdt, m_position, m_orientation, m_momentum, m_angularMomentum, m_orientationMatrix, m_velocity, m_angularVelocity);



	// Update the entities themselves.
	Rigidbody::Draw();
}

void Rigidbody::Draw()
{
	for (auto ent : m_entities) {
		ent->pos = m_position;
		ent->rotQuat = m_orientation;
	}
}

void Rigidbody::SetState(glm::vec3 position, glm::quat orientation, glm::vec3 momentum, glm::vec3 angularMomentum)
{
	m_position = position;
	m_orientation = orientation;
	m_momentum = momentum;
	m_angularMomentum = angularMomentum;
}

void Rigidbody::GetState(glm::vec3& position, glm::quat& orientation, glm::vec3& momentum, glm::vec3& angularMomentum) const
{
	position = m_position;
	orientation = m_orientation;
	momentum = m_momentum;
	angularMomentum = m_angularMomentum;
}

void Rigidbody::GetPosition(glm::vec3& position) const { position = m_position; }

void Rigidbody::SetForceFunction(Function force) { m_force = force; }

void Rigidbody::SetTorqueFunction(Function torque) { m_torque = torque; }

void Rigidbody::Convert(glm::quat Q, glm::vec3 P, glm::vec3 L, glm::mat3& R, glm::vec3& V, glm::vec3& W) const
{
	R = glm::toMat3(Q);
	V = m_invMass * P;
	W = R * m_bodyInvInertia * glm::transpose(R) * L; // J(t)^-1 = R(t) * J_body^-1 * R(t)^T
}

// Mesh/entity getters.
glm::vec3 Rigidbody::GetAxis(int best) const { 
	// 0-2 returns normal axis, 3-5 returns negative normal axis.
	return ((float) (-2 * (best / 3) + 1)) * static_cast<glm::vec3>(m_entity->GetModelMatrix()[best % 3]); 
}
glm::vec3 Rigidbody::GetSupport(glm::vec3 v) const { return m_position + (glm::sign(v) * m_halfwidth); }	// Support point equation for cuboid.
const glm::mat4 Rigidbody::GetModelMatrix() const { return m_entity->GetModelMatrix(); }
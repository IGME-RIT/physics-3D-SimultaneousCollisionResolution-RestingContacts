#pragma once

// Collision Manager is in charge of checking collisions between objects.
// It takes in Rigidbodies as inputs.
// Most of the code/logic is taken from the book 
// Game Physics Engine Development, 2nd Edition by Ian Millington.
// The repo for that book is here: https://github.com/idmillington/cyclone-physics
//
// Written by Chris Hambacher, 2021.

#include "Rigidbody.h"
#include "GTE/Mathematics/GMatrix.h"	// Matrix of any size (as GLM only allows for matrix of size 4 or smaller).
#include "GTE/Mathematics/LCPSolver.h"
#include <vector>

#define GTE_USE_ROW_MAJOR 1	// Used to tell GMatrix to use row major matrices.


namespace Collisions {

	// Class to store a contact point. This gets used by ContactData which stores contact points.
	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/include/cyclone/contacts.h#L59
	// This version has been modified to account for storing Rigidbodies and edges. If it's a VF contact, the two edges are invalid.
	// If it's an edge edge contact, all fields are valid.
	class Contact {
	public:
		Rigidbody* bodyOne = nullptr;
		Rigidbody* bodyTwo = nullptr;
		glm::vec3 contactPoint = glm::vec3(0);
		glm::vec3 contactNormal = glm::vec3(0);
		float penetrationDepth = 0.f;
		glm::vec3 edgeOne = glm::vec3(0);
		glm::vec3 edgeTwo = glm::vec3(0);
		// Is it a vertex-face contact (true) or edge edge contact (false).
		bool isVFContact = false;			
	};

	// Stores a number of contact points on a plane.
	// Taken from GDC2015 talk by Dirk Gregorius
	struct ContactManifold {
		int PointCount = 0;
		std::vector<Contact> Points;
		glm::vec3 Normal = glm::vec3(0);
	};

#pragma region Collision Detection Functions


	// We don't return a collision data, as this is just a preliminary check.
	// Call this before BoxBox.
	bool BoundingSphere(
		const Rigidbody& one,
		const Rigidbody& two
	);

	// Hull based SAT, taken from GDC 2015 talk by Dirk Gregorius.
	// The functions and code were designed to work with cuboids only, but
	// can be modified to work with general hulls.
	// http://media.steampowered.com/apps/valve/2015/DirkGregorius_Contacts.pdf
	void SAT(Rigidbody& one, Rigidbody& two, ContactManifold& manifold);
}

// Helper functions for our implementation of SAT.
namespace {

	// Check if there's no collision between the faces
	// of the two Rigidbodies (must be called twice).
	void QueryFaceDirections
	(
		const Rigidbody& one, 
		const Rigidbody& two, 
		float& largestPen, 
		unsigned& largestPenIndex
	);

	// Check if there's no collision between the edges
	// of the two Rigidbodies (only call once).
	void QueryEdgeDirections
	(
		const Rigidbody& one, 
		const Rigidbody& two, 
		float& largestPen, 
		glm::vec3& oneEdgeDirection, 
		glm::vec3& oneEdgePoint, 
		glm::vec3& twoEdgeDirection,
		glm::vec3& twoEdgePoint,
		glm::vec3& collisionAxis
	);

	// If there's a face-face contact, create the
	// contact manifold from given information.
	void CreateFaceContact
	(
		Collisions::ContactManifold& manifold,
		Rigidbody& one,
		const float& aLargestPen,
		const unsigned& aLargestPenIndex,
		Rigidbody& two,
		const float& bLargestPen,
		const unsigned& bLargestPenIndex
	);

	// If there's a edge-edge contact, create the
	// contact manifold from given information.
	void CreateEdgeContact
	(
		Collisions::ContactManifold& manifold,
		Rigidbody& one,
		Rigidbody& two,
		const float& edgeLargestPen,
		glm::vec3& oneEdgeDirection,
		glm::vec3& oneEdgePoint,
		glm::vec3& twoEdgeDirection,
		glm::vec3& twoEdgePoint,
		glm::vec3& collisionAxis
	);
}
#pragma endregion Collision Detection Functions

namespace Collisions {
// Collision resolution ideas taken from the book "Game Physics", by David Eberly.
#pragma region Collision Resolution Functions
	// Function that takes in a list of ALL collisions in the scene, and generates an LCP matrix based on them.
	// Matrix is a square matrix with num of rows/cols equal to number of collisions, and is upper triangular.
	void ComputeLCPMatrix(const std::vector<Collisions::Contact>& contacts, std::vector<float>& lcpMatrix);
	
	// Function that compues the preimpulse velocities.
	// Function uses GVector for output instead of normal std::vector, as we might have to use operations between GMatrix and GVector.
	void ComputePreImpulseVelocity(const std::vector<Collisions::Contact>& contacts, std::vector<float>& ddot);

	// Function that computes the vector b for resting contact points.
	void ComputeRestingContactVector(const std::vector<Collisions::Contact>& contacts, std::vector<float>& b);

	// Function that replaces preimpulse velocities with postimpulse velocities.
	void DoImpulse(const std::vector<Collisions::Contact>& contacts, std::vector<float>& f);

	// Function that actually applies forces to remove collisions. 
	void DoMotion(double t, double dt, const std::vector<Collisions::Contact>& contacts, std::vector<float> g);

	// Minimize |A * f + b|^2. Generate LCP problem from inputs A and dneg, output dpos and f vectors.
	void Minimize(const std::vector<float>& A, const std::vector<float>& dneg, std::vector<float>& dpos, std::vector<float>& f);

#pragma endregion Collision Resolution Functions
}

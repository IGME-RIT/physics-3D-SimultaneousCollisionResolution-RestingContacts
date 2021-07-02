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
		bool isVFContact = false;			// Is it a vertex-face contact (true) or edge edge contact (false).
	};

	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/include/cyclone/collide_fine.h#L183
	class CollisionData {
	public:
		Contact* contactArray;		// Points to the first element of the array.
		Contact* contacts;			// Points to the next blank element of the array.
		int contactsLeft;			// Number of remaining contacts the array can take.
		unsigned numOfContacts;		// Total number of contacts already in the array.

		CollisionData(unsigned maxContacts) {
			contactArray = new Contact[maxContacts];
			contacts = contactArray;
			contactsLeft = maxContacts;
			numOfContacts = 0;
		}
		~CollisionData() {
			delete[] contactArray;
			contactArray = nullptr;
			contacts = nullptr;
		}

		void AddContacts(unsigned count) {
			contactsLeft -= count;
			numOfContacts += count;

			contacts += count;	// Move array forward.
		}

		bool IsFull() {
			return contactsLeft <= 0;
		}
	};

	// Taken from GDC2015 talk by Dirk Gregorius
	struct ContactManifold {
		int PointCount;
		Contact Points[4];
		glm::vec3 Normal;
	};

#pragma region Collision Detection Functions



	// Collisions between two cuboids, returns bool and penetration data utilizing SAT.
	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L409
	bool BoxBox(
		const float& t,
		const float& dt,
		Rigidbody& one,
		Rigidbody& two,
		Collisions::CollisionData* data
	);

	// We don't return a collision data, as this is just a preliminary check.
	// Call this before BoxBox.
	bool BoundingSphere(
		const Rigidbody& one,
		const Rigidbody& two
	);

	// Hull based SAT, taken from GDC 2015 talk by Dirk Gregorius.
	void SAT(Rigidbody& one, Rigidbody& two, ContactManifold& manifold);
}
namespace {
	void QueryFaceDirections(const Rigidbody& one, const Rigidbody& two, float& largestPen, unsigned& largestPenIndex);
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

// Private namespace for helper functions.
namespace {
#pragma region Collision Resolution Helper Functions



#pragma endregion Collision Resolution Helper Functions

	// New SAT helper functions.
	void QueryFaceDirections(const Rigidbody& one, const Rigidbody& two, float& largestPen, float& largestPenIndex);
	void QueryEdgeDirections(const Rigidbody& one, const Rigidbody& two, float& largestPen, float& largestPenIndex);


#pragma region Collision Detection Helper Functions





	inline glm::vec3 contactPoint(
		const glm::vec3& pOne,
		const glm::vec3& dOne,
		float oneSize,
		const glm::vec3& pTwo,
		const glm::vec3& dTwo,
		float twoSize,

		// If this is true, and the contact point is outside
		// the edge (in the case of an edge-face contact) then
		// we use one's midpoint, otherwise we use two's.
		bool useOne
	);

	inline void fillPointFaceBoxBox(
		const Rigidbody& one,
		const Rigidbody& two,
		const glm::vec3& toCenter,
		Collisions::CollisionData* data,
		unsigned best,
		float pen
	);

	inline float projectOnAxis(
		const Rigidbody& rb,
		const glm::mat3& model,
		const glm::vec3& axis
	);


	inline bool tryAxis(
		Rigidbody& one,
		const glm::mat3& oneModel,
		Rigidbody& two,
		const glm::mat3& twoModel,
		glm::vec3 axis,					// Non-const as we normalize it.
		unsigned index,
		const glm::vec3& toCenter,		// Passed in to avoid lots of recalculation.
		const glm::vec3& relativeVel,
		const float& t,
		const float& dt,
		// Values that can be updated
		float& smallestPenetration,
		unsigned& smallestIndex,
		float& earliestTimeOfCollision,
		float& latestTimeOfCollision
	);
#pragma endregion Collision Detection Helper Functions
}

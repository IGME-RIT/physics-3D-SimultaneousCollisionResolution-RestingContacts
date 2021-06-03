#pragma once

// Collision Manager is in charge of checking collisions between objects.
// It takes in Rigidbodies as inputs.
// Most of the code/logic is taken from the book 
// Game Physics Engine Development, 2nd Edition by Ian Millington.
// The repo for that book is here: https://github.com/idmillington/cyclone-physics
//
// Written by Chris Hambacher, 2021.

#include "Rigidbody.h"

namespace Collisions {
	// Class to store a contact point. This gets used by ContactData which stores contact points.
	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/include/cyclone/contacts.h#L59
	class Contact {
	public:
		glm::vec3 contactPoint;
		glm::vec3 contactNormal;
		float penetrationDepth;
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

	// Collisions between two cuboids, returns bool and penetration data utilizing SAT.
	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L409
	bool BoxBox (
		const Rigidbody& one,
		const Rigidbody& two,
		Collisions::CollisionData* data
	);

}

// Private namespace for helper functions.
namespace {
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

	inline float penetrationOnAxis(
		const Rigidbody& one,
		const glm::mat3& oneModel,
		const Rigidbody& two,
		const glm::mat3& twoModel,
		const glm::vec3& axis,
		const glm::vec3& toCenter
	);

	inline bool tryAxis(
		const Rigidbody& one,
		const glm::mat3& oneModel,
		const Rigidbody& two,
		const glm::mat3& twoModel,
		glm::vec3 axis,					// Non-const as we normalize it.
		const glm::vec3& toCenter,		// Passed in to avoid lots of recalculation.
		unsigned index,
		// Values that can be updated
		float& smallestPenetration,
		unsigned& smallestIndex
	);
}

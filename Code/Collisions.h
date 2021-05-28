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

	// Collisions between two cuboids, returns bool and penetration data utilizing SAT.
	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L409
	bool BoxBox (
		const Rigidbody& one,
		const Rigidbody& two
	);
}

// Private namespace for helper functions.
namespace {
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


#include "Collisions.h"
#include "glm/gtx/norm.hpp"	// Some "experimental" math functions (aka I'm too lazy to code square length myself)
#include "glm/gtx/normalize_dot.hpp" // For fastNormalize.

// THINGS TO DO
// Implement collision data class.



namespace Collisions {
	bool BoxBox(
		const Rigidbody& one,
		const Rigidbody& two
	)
	{
		// Update the model matrices of each rigidbody (not sure if necessary).
		const glm::mat3 oneModel = (glm::mat3) one.GetEntity()->GetModelMatrix();
		const glm::mat3 twoModel = (glm::mat3) two.GetEntity()->GetModelMatrix();
		const glm::vec3 toCenter = two.GetCenter() - one.GetCenter();

		// We assume they aren't penetrating to start.
		float pen = FLT_MAX;		// Amount the objects are penetrating
		unsigned best = 0xffffff;	// Index of the axis the best pen occured on.

		// Check the big six axes (return zero if they aren't colliding).
		if (!tryAxis(one, oneModel, two, twoModel, oneModel[0], toCenter, 0, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, oneModel[1], toCenter, 1, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, oneModel[2], toCenter, 2, pen, best)) return 0;

		if (!tryAxis(one, oneModel, two, twoModel, twoModel[0], toCenter, 3, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, twoModel[1], toCenter, 4, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, twoModel[2], toCenter, 5, pen, best)) return 0;

		// Store the best axis-major, in case we run into parallel edge collisions later.
		unsigned bestSingleAxis = best;

		// Check the last nine axes.
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[0], twoModel[0]), toCenter, 6, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[0], twoModel[1]), toCenter, 7, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[0], twoModel[2]), toCenter, 8, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[1], twoModel[0]), toCenter, 9, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[1], twoModel[1]), toCenter, 10, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[1], twoModel[2]), toCenter, 11, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[2], twoModel[0]), toCenter, 12, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[2], twoModel[1]), toCenter, 13, pen, best)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[2], twoModel[2]), toCenter, 14, pen, best)) return 0;

		// If we don't have a result at this point, something went wrong.
		assert(best != 0xffffff);

		// We have a collision, and we know which axis it was on, and we deal accordingly.
		if (best < 3) {

		}
		else if (best < 6) {

		}
		else {

		}
	}
}

// Private namespace for helper functions.
namespace {

	// Projects the largest halfwidth onto the given axis.
	// Returns
	inline float projectOnAxis(
		const Rigidbody& rb,
		const glm::mat3& model,
		const glm::vec3& axis
	)
	{
		// Might be able to optimize this, not sure.
		return
			rb.GetHalfwidth().x + glm::abs(glm::dot(glm::fastNormalize(model[0]), axis)) +
			rb.GetHalfwidth().y + glm::abs(glm::dot(glm::fastNormalize(model[1]), axis)) +
			rb.GetHalfwidth().z + glm::abs(glm::dot(glm::fastNormalize(model[2]), axis));
	}


	// Function that actually checks if two boxes overlap. Call tryAxis instead.
	// Taken from https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L265
	inline float penetrationOnAxis(
		const Rigidbody& one,
		const glm::mat3& oneModel,
		const Rigidbody& two,
		const glm::mat3& twoModel,
		const glm::vec3& axis,
		const glm::vec3& toCenter
	)
	{
		// Project halfwidths onto axis.
		float oneProj = projectOnAxis(one, oneModel, axis);
		float twoProj = projectOnAxis(two, twoModel, axis);

		// Project distance between centers onto axis.
		float centerProj = glm::abs(glm::dot(toCenter, axis));

		// Return the overlap (positive means overlap, negative is no overlap).
		return oneProj + twoProj - centerProj;
	}


	// Function that determines if the two input rigidbodies are colliding along the given axis.
	// Taken from https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L285
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
	)
	{
		// Make sure the given axis wasn't generated from two near parallel axes.
		if (glm::length2(axis) > 0.001) return true;
		axis = glm::normalize(axis);

		float penetration = penetrationOnAxis(one, oneModel, two, twoModel, axis, toCenter);

		if (penetration < 0) return false;	// They aren't penetrating.
		if (penetration < smallestPenetration) {	// We found our new smallest penetration.
			smallestPenetration = penetration;
			smallestIndex = index;
		}
		return true;

	}
}
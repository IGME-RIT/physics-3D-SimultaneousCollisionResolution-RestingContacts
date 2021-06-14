
#include "Collisions.h"
#include "glm/gtx/norm.hpp"	// Some "experimental" math functions (aka I'm too lazy to code square length myself, glm::length2)
#include "glm/gtx/normalize_dot.hpp" // For fastNormalize.
#include "GTE/Mathematics/LCPSolver.h"	// LCP solver :)


namespace Collisions {

#pragma region Collision Detection Functions

	// Collisions between two cuboids, returns bool and penetration data utilizing SAT.
	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L409
	bool Collisions::BoxBox (
		const Rigidbody& one,
		const Rigidbody& two,
		Collisions::CollisionData* data
	)
	{
		// Update the model matrices of each rigidbody (not sure if necessary).
		const glm::mat3 oneModel = (glm::mat3) one.GetEntity()->GetModelMatrix();
		const glm::mat3 twoModel = (glm::mat3) two.GetEntity()->GetModelMatrix();
		const glm::vec3 toCenter = two.GetEntity()->GetWorldPosition() - one.GetEntity()->GetWorldPosition();

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
			// Vertex face collision, face on box one, vertex on box two.
			fillPointFaceBoxBox(one, two, toCenter, data, best, pen);
			data->AddContacts(1);
			return 1;
		}
		else if (best < 6) {
			// Vertex face collision, face on box two, vertex on box one.
			fillPointFaceBoxBox(two, one, toCenter * -1.0f, data, best - 3, pen);
			data->AddContacts(1);
			return 1;
		}
		else {
			// Edge edge collision.
			best -= 6;
			unsigned oneAxisIndex = best / 3;
			unsigned twoAxisIndex = best % 3;
			glm::vec3 oneAxis = one.GetAxis(oneAxisIndex);
			glm::vec3 twoAxis = two.GetAxis(twoAxisIndex);
			glm::vec3 axis = glm::cross(oneAxis, twoAxis);
			axis = glm::normalize(axis);

			// The axis should point from box one to box two.
			if (glm::dot(axis, toCenter) > 0) axis = axis * -1.0f;

			// We have the axes, but not the edges: each axis has 4 edges parallel
			// to it, we need to find which of the 4 for each object. We do
			// that by finding the point in the centre of the edge. We know
			// its component in the direction of the box's collision axis is zero
			// (its a mid-point) and we determine which of the extremes in each
			// of the other axes is closest.
			glm::vec3 ptOnOneEdge = one.GetHalfwidth();
			glm::vec3 ptOnTwoEdge = two.GetHalfwidth();
			for (unsigned i = 0; i < 3; i++)
			{
				if (i == oneAxisIndex) ptOnOneEdge[i] = 0;
				else if (glm::dot(one.GetAxis(i), axis) > 0) ptOnOneEdge[i] = -ptOnOneEdge[i];

				if (i == twoAxisIndex) ptOnTwoEdge[i] = 0;
				else if (glm::dot(two.GetAxis(i), axis) < 0) ptOnTwoEdge[i] = -ptOnTwoEdge[i];
			}

			// Get both of the edges as normalized vectors in world space (to pass into the contact).
			glm::vec3 oneEdge = ptOnOneEdge;
			oneEdge[oneAxisIndex] = one.GetHalfwidth()[oneAxisIndex];
			oneEdge = static_cast<glm::vec3>(one.GetModelMatrix() * glm::vec4(oneEdge, 0));	// Zero because we care about direction, not location.
			oneEdge = glm::normalize(oneEdge);

			glm::vec3 twoEdge = ptOnTwoEdge;
			twoEdge[oneAxisIndex] = two.GetHalfwidth()[twoAxisIndex];
			twoEdge = static_cast<glm::vec3>(two.GetModelMatrix() * glm::vec4(twoEdge, 0));
			twoEdge = glm::normalize(twoEdge);

			// Move them into world coordinates (they are already oriented
			// correctly, since they have been derived from the axes).
			// I think they aren't actually oriented correctly, as halfwidth isn't oriented. Multiply by model matrix instead.
			ptOnOneEdge = static_cast<glm::vec3>(one.GetModelMatrix() * glm::vec4(ptOnOneEdge, 1));
			ptOnTwoEdge = static_cast<glm::vec3>(two.GetModelMatrix() * glm::vec4(ptOnTwoEdge, 1));

			// So we have a point and a direction for the colliding edges.
			// We need to find out point of closest approach of the two
			// line-segments.
			glm::vec3 vertex = contactPoint(
				ptOnOneEdge, oneAxis, one.GetHalfwidth()[oneAxisIndex],
				ptOnTwoEdge, twoAxis, two.GetHalfwidth()[twoAxisIndex],
				bestSingleAxis > 2
			);

			// We can fill the contact.
			Contact* contact = data->contacts;

			contact->penetrationDepth = pen;
			contact->contactNormal = axis;
			contact->contactPoint = vertex;

			contact->edgeOne = oneEdge;
			contact->edgeTwo = twoEdge;
			contact->isVFContact = false;

			data->AddContacts(1);
			return 1;
		}

		return 0;
	}

	bool BoundingSphere(const Rigidbody& one, const Rigidbody& two)
	{
		glm::vec3 oneCenter = one.GetEntity()->GetWorldPosition();
		glm::vec3 twoCenter = two.GetEntity()->GetWorldPosition();

		if (glm::length2(twoCenter - oneCenter) > pow(one.GetRadius() + two.GetRadius(), 2))
			return false;
		return true;
	}

#pragma endregion Collision Detection Functions



#pragma region Resting Contacts Collision Resolution Functions
	void ComputeLCPMatrix(std::vector<Collisions::Contact> contacts, gte::GMatrix<float>& A)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Contact ci = contacts[i];
			glm::vec3 rANi = glm::cross(ci.contactPoint - ci.bodyOne->GetAxis(0), ci.contactNormal);
			glm::vec3 rBNi = glm::cross(ci.contactPoint - ci.bodyTwo->GetAxis(0), ci.contactNormal);

			for (int j = 0; j < contacts.size(); j++) {
				Contact cj = contacts[j];
				glm::vec3 rANj = glm::cross(cj.contactPoint - cj.bodyOne->GetAxis(0), cj.contactNormal);
				glm::vec3 rBNj = glm::cross(cj.contactPoint - cj.bodyTwo->GetAxis(0), cj.contactNormal);

				A(i, j) = 0;

				if (ci.bodyOne == cj.bodyOne) {
					// personal note: massinv is 1/mass, jinv is inverse inertia tensor.
					A(i, j) += ci.bodyOne->GetInverseMass() * glm::dot(ci.contactNormal, cj.contactNormal);
					A(i, j) += glm::dot(rANi, ci.bodyOne->GetInverseIntertia() * rANj);
				}
				else if (ci.bodyOne == cj.bodyTwo) {
					A(i, j) -= ci.bodyOne->GetInverseMass() * glm::dot(ci.contactNormal, cj.contactNormal);
					A(i, j) -= glm::dot(rANi, ci.bodyOne->GetInverseIntertia() * rANj);
				}

				if (ci.bodyTwo == cj.bodyOne) {
					A(i, j) += ci.bodyTwo->GetInverseMass() * glm::dot(ci.contactNormal, cj.contactNormal);
					A(i, j) += glm::dot(rBNi, ci.bodyTwo->GetInverseIntertia() * rBNj);
				}
				else if (ci.bodyTwo == cj.bodyTwo) {
					A(i, j) -= ci.bodyTwo->GetInverseMass() * glm::dot(ci.contactNormal, cj.contactNormal);
					A(i, j) -= glm::dot(rBNi, ci.bodyTwo->GetInverseIntertia() * rBNj);
				}
			}
		}


	}

	void ComputePreImpulseVelocity(std::vector<Collisions::Contact> contacts, gte::GVector<float>& ddot)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Contact ci = contacts[i];
			
			glm::vec3 velA = ci.bodyOne->GetVelocity() + glm::cross(ci.bodyOne->GetAngularVelocity(), ci.contactPoint - ci.bodyOne->GetPosition());
			glm::vec3 velB = ci.bodyTwo->GetVelocity() + glm::cross(ci.bodyTwo->GetAngularVelocity(), ci.contactPoint - ci.bodyTwo->GetPosition());
			ddot[i] = glm::dot(ci.contactNormal, velA - velB);
		}

	}

	void ComputeRestingContactVector(std::vector<Collisions::Contact> contacts, gte::GVector<float>& b)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Contact ci = contacts[i];
			Rigidbody* A = ci.bodyOne;
			Rigidbody* B = ci.bodyTwo;

			// Body one terms.
			glm::vec3 rAi = ci.contactPoint - A->GetPosition();
			glm::vec3 wAxrAi = glm::cross(A->GetAngularVelocity(), rAi);
			glm::vec3 At1 = A->GetInverseMass() * A->GetForce();
			glm::vec3 At2 = glm::cross(A->GetInverseIntertia() * (A->GetTorque() + glm::cross(A->GetAngularMomentum(), A->GetAngularVelocity())), rAi);
			glm::vec3 At3 = glm::cross(A->GetAngularVelocity(), wAxrAi);
			glm::vec3 At4 = A->GetVelocity() + wAxrAi;

			// Body two terms.
			glm::vec3 rBi = ci.contactPoint - B->GetPosition();
			glm::vec3 wBxrBi = glm::cross(B->GetAngularVelocity(), rBi);
			glm::vec3 Bt1 = B->GetInverseMass() * B->GetForce();
			glm::vec3 Bt2 = glm::cross(B->GetInverseIntertia() * (B->GetTorque() + glm::cross(B->GetAngularMomentum(), B->GetAngularVelocity())), rBi);
			glm::vec3 Bt3 = glm::cross(B->GetAngularVelocity(), wBxrBi);
			glm::vec3 Bt4 = B->GetVelocity() + wBxrBi;

			// Compute derivative of contact normal.
			glm::vec3 Ndot;
			if (ci.isVFContact) {
				Ndot = glm::cross(B->GetAngularVelocity(), ci.contactNormal);
			}
			else {
				glm::vec3 EdgeOnedot = glm::cross(A->GetAngularVelocity(), ci.edgeOne);
				glm::vec3 EdgeTwodot = glm::cross(B->GetAngularVelocity(), ci.edgeTwo);
				glm::vec3 U = glm::cross(ci.edgeOne, EdgeTwodot) + glm::cross(EdgeOnedot, ci.edgeTwo);
				Ndot = (U - glm::dot(U, ci.contactNormal)) * glm::normalize(ci.contactNormal);
			}

			// Compute b vector element.
			b[i] = glm::dot(ci.contactNormal, At1 + At2 + At3 - Bt1 - Bt2 - Bt3) + (2.f * glm::dot(Ndot, At4 - Bt4));
		}
	}

	void DoImpulse(std::vector<Collisions::Contact> contacts, gte::GVector<float>& f)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Collisions::Contact ci = contacts[i];
			Rigidbody::State& A = ci.bodyOne->GetState();
			Rigidbody::State& B = ci.bodyOne->GetState();

			// Update momentum.
			glm::vec3 impulse = f[i] * ci.contactNormal;
			A.momentum += impulse;
			B.momentum -= impulse;
			A.angularMomentum += glm::cross(ci.contactPoint - A.pos, impulse);
			B.angularMomentum -= glm::cross(ci.contactPoint - B.pos, impulse);

			// Update velocity.
			// Don't need to?
		}
	}

#pragma endregion Resting Contacts Collision Resolution Functions

}

// Private namespace for helper functions.
namespace {

	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L349
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
	)
	{
		glm::vec3 toSt, cOne, cTwo;
		float dpStaOne, dpStaTwo, dpOneTwo, smOne, smTwo;
		float denom, mua, mub;

		smOne = glm::length2(dOne);
		smTwo = glm::length2(dTwo);
		dpOneTwo = glm::dot(dTwo, dOne);

		toSt = pOne - pTwo;
		dpStaOne = glm::dot(dOne, toSt);
		dpStaTwo = glm::dot(dTwo, toSt);

		denom = smOne * smTwo - dpOneTwo * dpOneTwo;

		// Zero denominator indicates parrallel lines
		if (abs(denom) < 0.0001f) {
			return useOne ? pOne : pTwo;
		}

		mua = (dpOneTwo * dpStaTwo - smTwo * dpStaOne) / denom;
		mub = (smOne * dpStaTwo - dpOneTwo * dpStaOne) / denom;

		// If either of the edges has the nearest point out
		// of bounds, then the edges aren't crossed, we have
		// an edge-face contact. Our point is on the edge, which
		// we know from the useOne parameter.
		if (mua > oneSize ||
			mua < -oneSize ||
			mub > twoSize ||
			mub < -twoSize)
		{
			return useOne ? pOne : pTwo;
		}
		else
		{
			cOne = pOne + dOne * mua;
			cTwo = pTwo + dTwo * mub;

			return cOne * 0.5f + cTwo * 0.5f;
		}
	}

	// Method is called when we know we have vertex-face collision.
	// Pulled from: https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L311
	inline void fillPointFaceBoxBox(
		const Rigidbody& one,
		const Rigidbody& two,
		const glm::vec3& toCenter,
		Collisions::CollisionData* data,
		unsigned best,
		float pen
	)
	{
		Collisions::Contact* contact = data->contacts;

		// We know axis, but which of the two faces is the collision on.
		glm::vec3 normal = one.GetAxis(best);
		if (glm::dot(normal, toCenter) > 0) {
			normal *= -1.0f;
		}

		// Figure out the vertex that is colliding (just using toCenter doesn't work!)
		glm::vec3 vertex = two.GetHalfwidth();
		if (glm::dot(two.GetAxis(0), normal) < 0) vertex.x *= -1.0f;
		if (glm::dot(two.GetAxis(1), normal) < 0) vertex.y *= -1.0f;
		if (glm::dot(two.GetAxis(2), normal) < 0) vertex.z *= -1.0f;

		// Create the contat data.
		contact->contactNormal = normal;
		contact->penetrationDepth = pen;
		// Convert from local space to world, accounting for translations.
		glm::mat4 twoModelMatrix = two.GetModelMatrix();	// Make copy of the matrix.
		contact->contactPoint = static_cast<glm::vec3>(twoModelMatrix * glm::vec4(vertex, 1));

		contact->isVFContact = true;	// It's a vertex-face contact.
	}

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
			rb.GetHalfwidth().x * glm::abs(glm::dot(glm::fastNormalize(model[0]), axis)) +
			rb.GetHalfwidth().y * glm::abs(glm::dot(glm::fastNormalize(model[1]), axis)) +
			rb.GetHalfwidth().z * glm::abs(glm::dot(glm::fastNormalize(model[2]), axis));
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


	// Function that determinees if the two input rigidbodies are colliding along the given axis.
	// Taken from https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L285
	bool tryAxis(
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
		if (glm::length2(axis) < 0.001) return true;
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
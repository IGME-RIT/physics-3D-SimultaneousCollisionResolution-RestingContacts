
#include "Collisions.h"
#include "glm/gtx/norm.hpp"	// Some "experimental" math functions (aka I'm too lazy to code square length myself, glm::length2)
#include "glm/gtx/normalize_dot.hpp" // For fastNormalize.
#include "GTE/Mathematics/LCPSolver.h"	// LCP solver :)
#include <iostream>

// If we detect penetration/non-penetration within this threshold, we have a contact.
#define COLLISION_THRESHOLD 0.01f

namespace Collisions {

#pragma region OLD Collision Detection Functions


	// Collisions between two cuboids, returns bool and penetration data utilizing SAT.
	// https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L409
	bool Collisions::BoxBox(
		const float& t,
		const float& dt,
		Rigidbody& one,
		Rigidbody& two,
		Collisions::CollisionData* data
	)
	{
		// Update the model matrices of each rigidbody (not sure if necessary).
		const glm::mat3 oneModel = (glm::mat3) one.GetModelMatrix();
		const glm::mat3 twoModel = (glm::mat3) two.GetModelMatrix();
		const glm::vec3 toCenter = two.m_position - one.m_position;
		const glm::vec3 relativeVelocity = two.m_velocity - one.m_velocity;


		// We assume they aren't penetrating to start.
		float pen = FLT_MAX;		// Amount the objects are penetrating
		unsigned best = 0xffffff;	// Index of the axis the best pen occured on.
		float earliestTimeOfCollision = -FLT_MAX;
		float latestTimeOfCollision = FLT_MAX;

		// Check the big six axes (return zero if they aren't colliding).
		if (!tryAxis(one, oneModel, two, twoModel, oneModel[0], 0, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, oneModel[1], 1, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, oneModel[2], 2, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		
		if (!tryAxis(one, oneModel, two, twoModel, twoModel[0], 3, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, twoModel[1], 4, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, twoModel[2], 5, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		
		// Store the best axis-major, in case we run into parallel edge collisions later.
		unsigned bestSingleAxis = best;
		
		// Check the last nine axes.
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[0], twoModel[0]), 6, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[0], twoModel[1]), 7, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[0], twoModel[2]), 8, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[1], twoModel[0]), 9, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[1], twoModel[1]), 10, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[1], twoModel[2]), 11, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[2], twoModel[0]), 12, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[2], twoModel[1]), 13, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;
		if (!tryAxis(one, oneModel, two, twoModel, glm::cross(oneModel[2], twoModel[2]), 14, toCenter, relativeVelocity, t, dt, pen, best, earliestTimeOfCollision, latestTimeOfCollision)) return 0;


		// They must be colliding or will be colliding. If the predicted time of collision is within the next frame, set the rigidbodies dt to take a partial step.
		// UPDATE: This is done in tryAxis for early outs.
		//std::cout << "t, earliest = " << t << ", " << earliestTimeOfCollision << std::endl;
		//if (earliestTimeOfCollision > t && earliestTimeOfCollision < t + dt) {
		//	float new_dt = earliestTimeOfCollision - t;
		//	if (new_dt < one.m_dt) one.m_dt = new_dt;
		//	if (new_dt < two.m_dt) two.m_dt = new_dt;
		//}
		assert(best != 0xffffff);
		
		// We have a collision, and we know which axis it was on, and we deal accordingly.
		if (best < 3) {
			// Vertex face collision, face on box one, vertex on box two.
			fillPointFaceBoxBox(one, two, toCenter, data, best, pen);
			data->contacts->bodyOne = &one;
			data->contacts->bodyTwo = &two;
			data->AddContacts(1);
			return 1;
		}
		else if (best < 6) {
			// Vertex face collision, face on box two, vertex on box one.
			fillPointFaceBoxBox(two, one, toCenter * -1.0f, data, best - 3, pen);
			data->contacts->bodyOne = &one;
			data->contacts->bodyTwo = &two;
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
			glm::vec3 ptOnOneEdge, ptOnTwoEdge;
			ptOnOneEdge = one.m_halfwidth;
			ptOnTwoEdge = two.m_halfwidth;
			for (unsigned i = 0; i < 3; i++)
			{
				if (i == oneAxisIndex) ptOnOneEdge[i] = 0;
				else if (glm::dot(one.GetAxis(i), axis) > 0) ptOnOneEdge[i] = -ptOnOneEdge[i];

				if (i == twoAxisIndex) ptOnTwoEdge[i] = 0;
				else if (glm::dot(two.GetAxis(i), axis) < 0) ptOnTwoEdge[i] = -ptOnTwoEdge[i];
			}

			// Get both of the edges as normalized vectors in world space (to pass into the contact).
			glm::vec3 oneEdge = ptOnOneEdge;
			oneEdge[oneAxisIndex] = one.m_halfwidth[oneAxisIndex];
			oneEdge = static_cast<glm::vec3>(one.GetModelMatrix() * glm::vec4(oneEdge, 0));	// Zero because we care about direction, not location.
			oneEdge = glm::normalize(oneEdge);

			glm::vec3 twoEdge = ptOnTwoEdge;
			twoEdge[oneAxisIndex] = two.m_halfwidth[twoAxisIndex];
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
				ptOnOneEdge, oneAxis, one.m_halfwidth[oneAxisIndex],
				ptOnTwoEdge, twoAxis, two.m_halfwidth[twoAxisIndex],
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

			contact->bodyOne = &one;
			contact->bodyTwo = &two;

			data->AddContacts(1);
			return 1;
		}
	}

	bool BoundingSphere(const Rigidbody& one, const Rigidbody& two)
	{
		float a = glm::length2(two.m_position - one.m_position);
		float b = pow(one.m_radius + two.m_radius, 2);
		if (glm::length2(two.m_position - one.m_position) > pow(one.m_radius + two.m_radius, 2))
			return false;
		return true;
	}
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
		glm::vec3 vertex = two.m_halfwidth;
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
			rb.m_halfwidth.x * glm::abs(glm::dot(glm::fastNormalize(model[0]), axis)) +
			rb.m_halfwidth.y * glm::abs(glm::dot(glm::fastNormalize(model[1]), axis)) +
			rb.m_halfwidth.z * glm::abs(glm::dot(glm::fastNormalize(model[2]), axis));
	}


	// Function that determinees if the two input rigidbodies are colliding along the given axis.
	// Taken from https://github.com/idmillington/cyclone-physics/blob/d75c8d9edeebfdc0deebe203fe862299084b1e30/src/collide_fine.cpp#L285
	// Adjusted to account for time of collision, logic from here: https://pybullet.org/Bullet/phpBB3/viewtopic.php?t=138

	/*  
		* LOGIC: Against the specified axis, we project both objects and the relative velocity. At t0, if the objects are seperated and moving
		* apart, we know that the two objects will not collide during this interval. Otherwise, we compute the earliest future time of collision,
		* and the latest future time where they will stop colliding (if they are colliding at t0, the earliest time is t0). If the maximum earliest
		* future time from all the axes is less than the minimum latest future time from all the axes, the maximum earliest future intersection time
		* is the time of collision. Otherwise, there is no collision.
		*/ 
	bool tryAxis(
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
	)
	{
		// Make sure the given axis wasn't generated from two near parallel axes.
		if (glm::length2(axis) < 0.001) return true;
		axis = glm::normalize(axis);

		// Get the overlap (positive means overlap, negative is no overlap).
		float oneProj = projectOnAxis(one, oneModel, axis);
		float twoProj = projectOnAxis(two, twoModel, axis);
		float centerProj = glm::dot(toCenter, axis);
		float penetration = oneProj + twoProj - glm::abs(centerProj);

		// Project the relative velocity onto the axis to get speed.
		float projectedSpeed = centerProj < 0 ? glm::dot(two.m_velocity - one.m_velocity, axis) : glm::dot(one.m_velocity - two.m_velocity, axis);

		// EARLY OUT: If the objects are not overlapping and projected relative velocity is moving away, they will not collide.
		//std::cout << "Pen/Speed/axis: " << penetration << ", " << projectedSpeed << ", " << axis[0] << axis[1] << axis[2] << std::endl;
		if (penetration < -COLLISION_THRESHOLD && projectedSpeed <= 0) return false;

		// 0 = penetration + projectedSpeed * (earliestTimeOfCollision - t)
		float localEarliestTimeOfCollision = (-penetration / projectedSpeed) + t;

		// 2 * (oneProj + twoProj) = projectedSpeed * (latestTimeOfCollision - earliestTimeOfCollision)
		float localLatestTimeOfCollision = (2.f / projectedSpeed) * (oneProj + twoProj) + localEarliestTimeOfCollision;

		// Keep the maximum earliest and the minimum latest.
		if (localEarliestTimeOfCollision > earliestTimeOfCollision) earliestTimeOfCollision = localEarliestTimeOfCollision;
		if (localLatestTimeOfCollision < latestTimeOfCollision) latestTimeOfCollision = localLatestTimeOfCollision;

		// EARLY OUT: If the maximum earliest is greater than minimum latest of all axes tested so far, no future collision.
		if (earliestTimeOfCollision > latestTimeOfCollision) return false;

		// At this point, if pen < 0, we know they aren't colliding this frame.
		if (penetration < -COLLISION_THRESHOLD) {
			// If they will be colliding next frame, reduce the amount of distance travelled next frame.
			if (earliestTimeOfCollision > t && earliestTimeOfCollision < t + dt) {
				float new_dt = earliestTimeOfCollision - t;
				if (new_dt < one.m_dt) one.m_dt = new_dt;
				if (new_dt < two.m_dt) two.m_dt = new_dt;
				std::cout << "New dt for two objects: " << new_dt << std::endl;
			}
			return false;
		}

		if (penetration < smallestPenetration) {	// We found our new smallest penetration.
			smallestPenetration = penetration;
			smallestIndex = index;
		}

		return true;

	}
}
#pragma endregion OLD Collision Detection Functions

namespace Collisions {
#pragma region NEW Collision Detection Functions

	void SAT(const Rigidbody& one, const Rigidbody& two, ContactManifold& manifold)
	{
		unsigned AFaceQueryPenIndex = 0; 
		float AFaceQueryPen = -FLT_MAX;
		QueryFaceDirections(one, two, AFaceQueryPen, AFaceQueryPenIndex);
		if (AFaceQueryPen > 0.f) return;	// separating axis found.

		unsigned BFaceQueryPenIndex = 0;
		float BFaceQueryPen = -FLT_MAX;
		QueryFaceDirections(two, one, BFaceQueryPen, BFaceQueryPenIndex);
		if (BFaceQueryPen > 0.f) return;	// separating axis found.

		float CEdgeQueryPen = -FLT_MAX;
		glm::vec3 edgeDirection, edgePoint, supportPoint;
		QueryEdgeDirections(one, two, CEdgeQueryPen, edgeDirection, edgePoint, supportPoint);
		if (CEdgeQueryPen > 0.f) return;	// separating axis found.

		// Hulls must overlap.
		bool blsFaceContactA = AFaceQueryPen >= CEdgeQueryPen;
		bool blsFaceContactB = BFaceQueryPen >= CEdgeQueryPen;
		if (blsFaceContactA && blsFaceContactB)
			CreateFaceContact(manifold, one, AFaceQueryPen, AFaceQueryPenIndex, two, BFaceQueryPen, BFaceQueryPenIndex);
		else
			CreateEdgeContact(manifold, one, two, CEdgeQueryPen, edgeDirection, edgePoint, supportPoint);

	}






#pragma endregion NEW Collision Detection Functions
}

// Helper functions.
namespace {
	void QueryFaceDirections(const Rigidbody& one, const Rigidbody& two, float& largestPen, unsigned& largestPenIndex) {
		for (int index = 0; index < 6; ++index) {
			glm::vec3 planeNormalA = one.GetAxis(index);
			glm::vec3 planeCenterA = planeNormalA * one.m_halfwidth + one.m_position;
			glm::vec3 vertexB = two.GetSupport(-planeNormalA);
			float distance = glm::dot(planeNormalA, (vertexB - planeCenterA));	// If distance is greater than zero, we found seperating axis.

			if (largestPen < distance) {
				largestPen = distance;
				largestPenIndex = index;
			}
		}
	}

	void QueryEdgeDirections(const Rigidbody& one, const Rigidbody& two, float& largestPen, glm::vec3& edgeDirection, glm::vec3& edgeMidpoint, glm::vec3& supportPoint) {
		// Greatly simplifying this function due to dealing with cuboids (usually would have to test every edge pair for convex hull).
		const glm::mat3 oneModel = (glm::mat3) one.GetModelMatrix();
		const glm::mat3 twoModel = (glm::mat3) two.GetModelMatrix();
		
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				// Get the axis to project against.
				glm::vec3 axis = glm::cross(oneModel[i], twoModel[j]);
				if (glm::length2(axis) < 0.001f) continue;	// Skip near parallel edges.

				// Get all the center point of all four edges that point in this direction (in local space translation).
				std::vector<glm::vec3> localEdges;
				localEdges.push_back(( oneModel[(i + 1) % 3] + oneModel[(i + 2) % 3]) * one.m_halfwidth + one.m_position);
				localEdges.push_back(( oneModel[(i + 1) % 3] - oneModel[(i + 2) % 3]) * one.m_halfwidth + one.m_position);
				localEdges.push_back((-oneModel[(i + 1) % 3] + oneModel[(i + 2) % 3]) * one.m_halfwidth + one.m_position);
				localEdges.push_back((-oneModel[(i + 1) % 3] - oneModel[(i + 2) % 3]) * one.m_halfwidth + one.m_position);

				for (glm::vec3 edgePoint : localEdges) {
					// Make sure axis is the right direction for this edge.
					if (glm::dot(axis, edgePoint - one.m_position) < 0.f)
						axis = -axis;

					// Get the distance between plane and support vertex (plane has normal axis and ptOnEdge is located on plane).
					glm::vec3 vertexB = two.GetSupport(-axis);
					float distance = glm::dot(axis, (vertexB - edgePoint));

					if (largestPen < distance) {
						largestPen = distance;
						// Information about the collision (or lack thereof)
						edgeDirection = axis;
						edgeMidpoint = edgePoint;
						supportPoint = vertexB;
					}
				}




			}
		}
	}

	void CreateFaceContact
	(
		Collisions::ContactManifold& manifold, 
		const Rigidbody& one, 
		const float& aLargestPen,
		const unsigned& aLargestPenIndex,
		const Rigidbody& two,
		const float& bLargestPen,
		const unsigned& bLargestPenIndex
	) 
	{
		const Rigidbody* incidentBody; 
		const Rigidbody* referenceBody;
		glm::vec3 referenceFaceNormal, incidentFaceNormal;
		
		auto GenerateManifold = [&]
		(
			Collisions::ContactManifold& manifold,
			const Rigidbody& referenceBody,
			const Rigidbody& incidentBody,
			const unsigned& penIndex
		)
		{
			// Determine the reference and incident face.
			glm::vec3 referenceFaceNormal, incidentFaceNormal;
			glm::mat3 incidentModelMatrix;

			referenceFaceNormal = referenceBody.GetAxis(penIndex);
			incidentModelMatrix = (glm::mat3) incidentBody.GetModelMatrix();

			// Variables relating to the incident face and for determining.
			glm::vec3 testingFaceNormal;
			float smallestDot = FLT_MAX;
			unsigned indexOfIncidentFace;
			for (int i = 0; i < 6; ++i) {
				testingFaceNormal = incidentBody.GetAxis(i);
				if (glm::dot(testingFaceNormal, referenceFaceNormal) < smallestDot) {
					incidentFaceNormal = testingFaceNormal;
					indexOfIncidentFace = i;
				}
			}

			// Clip the incident plane against the side planes of the reference face.
			// We use Sutherland Hodgman clipping for this, and we keep all vertices below the reference plane.
			// Get the four points that compose the incident face.
			std::vector<glm::vec3> incidentFacePoints;
			{
				const unsigned& i = indexOfIncidentFace;
				incidentFacePoints.push_back((incidentFaceNormal + incidentModelMatrix[(i + 1) % 3] + incidentModelMatrix[(i + 2) % 3]) * incidentBody.m_halfwidth + incidentBody.m_position);
				incidentFacePoints.push_back((incidentFaceNormal + incidentModelMatrix[(i + 1) % 3] - incidentModelMatrix[(i + 2) % 3]) * incidentBody.m_halfwidth + incidentBody.m_position);
				incidentFacePoints.push_back((incidentFaceNormal - incidentModelMatrix[(i + 1) % 3] + incidentModelMatrix[(i + 2) % 3]) * incidentBody.m_halfwidth + incidentBody.m_position);
				incidentFacePoints.push_back((incidentFaceNormal - incidentModelMatrix[(i + 1) % 3] - incidentModelMatrix[(i + 2) % 3]) * incidentBody.m_halfwidth + incidentBody.m_position);
			}

			// Clip them against the four side planes.
			std::vector<glm::vec3> clippedFacePoints;
			for (int i = 0; i < 6; i++) {
				if (i == penIndex || i == (penIndex + 3) % 6) continue;
				glm::vec3 clippingPlaneNormal = referenceBody.GetAxis(i);
				glm::vec3 clippingPlanePoint = referenceBody.m_position + (clippingPlaneNormal * referenceBody.m_halfwidth);

				// Clip each edge.
				for (int j = 0; j < 4; j++) {
					glm::vec3& currentPoint = incidentFacePoints[j];
					glm::vec3& nextPoint = incidentFacePoints[(j + 1) % 4];

					// If the nextPoint is on the inside.
					if (glm::dot(nextPoint - clippingPlanePoint, clippingPlaneNormal) < 0.f) {
						// If the currentPoint is on the outside, add intersection point.
						if (glm::dot(currentPoint - clippingPlanePoint, clippingPlaneNormal) > 0.f) {
							// Calculate the intersection between plane and line segment.
							// Plane is of the form DOT(p - p_0, n) = 0, where p_0 is a point on the plane and n is the normal to the plane. 
							// Line segment is of the form p = l_0 + l * d, where l_0 is a point on the line and l is a direction vector.
							// Intersection point is l_0 + l * (DOT(p_0 - l_0, n)/DOT(l, n)).
							glm::vec3 l_0 = currentPoint;
							glm::vec3 l = nextPoint - currentPoint;
							clippedFacePoints.push_back(l_0 + l * (glm::dot(clippingPlanePoint - l_0, clippingPlaneNormal) / glm::dot(l, clippingPlaneNormal)));
						}
						clippedFacePoints.push_back(nextPoint);
					}
					// If the currentPoint is on the inside, just add the intersecting point.
					else if (glm::dot(currentPoint - clippingPlanePoint, clippingPlaneNormal) < 0.f) {
						glm::vec3 l_0 = currentPoint;
						glm::vec3 l = nextPoint - currentPoint;
						clippedFacePoints.push_back(l_0 + l * (glm::dot(clippingPlanePoint - l_0, clippingPlaneNormal) / glm::dot(l, clippingPlaneNormal)));
					}
				}

				// We replace our original set of points with the clipped one.
				// Every time we clip against a plane, this can add more points to our list, which we also want to clip against other planes.
				incidentFacePoints = clippedFacePoints;
				clippedFacePoints.clear();
			}
			std::cout << "hi";
		};

		if (aLargestPen < bLargestPen) {
			// B penetrates A
			GenerateManifold(manifold, one, two, aLargestPenIndex);
		}
		else {
			// A penetrates B
			GenerateManifold(manifold, two, one, bLargestPenIndex);
		}
	}

	void CreateEdgeContact
	(
		Collisions::ContactManifold& manifold,
		const Rigidbody& one,
		const Rigidbody& two,
		const float& edgeLargestPen,
		glm::vec3& edgeDirection, 
		glm::vec3& edgeMidpoint, 
		glm::vec3& supportPoint
	)
	{
		std::cout << "edge contact" << std::endl;
	}

}	// End of empty namespace.


namespace Collisions {
#pragma region Resting Contacts Collision Resolution Functions
	void ComputeLCPMatrix(const std::vector<Collisions::Contact>& contacts, std::vector<float>& A)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Contact ci = contacts[i];
			glm::vec3 rANi = glm::cross(ci.contactPoint - ci.bodyOne->m_position, ci.contactNormal);
			glm::vec3 rBNi = glm::cross(ci.contactPoint - ci.bodyTwo->m_position, ci.contactNormal);
			//glm::vec3 rANi = glm::normalize(glm::cross(ci.contactPoint - ci.bodyOne->m_position, ci.contactNormal));
			//glm::vec3 rBNi = glm::normalize(glm::cross(ci.contactPoint - ci.bodyTwo->m_position, ci.contactNormal));

			for (int j = 0; j < contacts.size(); j++) {
				Contact cj = contacts[j];
				glm::vec3 rANj = glm::cross(cj.contactPoint - cj.bodyOne->m_position, cj.contactNormal);
				glm::vec3 rBNj = glm::cross(cj.contactPoint - cj.bodyTwo->m_position, cj.contactNormal);
				//glm::vec3 rANj = glm::normalize(glm::cross(cj.contactPoint - cj.bodyOne->m_position, cj.contactNormal));
				//glm::vec3 rBNj = glm::normalize(glm::cross(cj.contactPoint - cj.bodyTwo->m_position, cj.contactNormal));

				float& A_ij = A[i * contacts.size() + j];
				A_ij = 0;

				if (ci.bodyOne == cj.bodyOne) {
					A_ij += ci.bodyOne->m_invMass * glm::dot(ci.contactNormal, cj.contactNormal);
					A_ij += glm::dot(rANi, ci.bodyOne->m_invInertia * rANj);
				}
				else if (ci.bodyOne == cj.bodyTwo) {
					A_ij -= ci.bodyOne->m_invMass * glm::dot(ci.contactNormal, cj.contactNormal);
					A_ij -= glm::dot(rANi, ci.bodyOne->m_invInertia * rANj);
				}

				if (ci.bodyTwo == cj.bodyOne) {
					A_ij -= ci.bodyTwo->m_invMass * glm::dot(ci.contactNormal, cj.contactNormal);
					A_ij -= glm::dot(rBNi, ci.bodyTwo->m_invInertia * rBNj);
				}
				else if (ci.bodyTwo == cj.bodyTwo) {
					A_ij += ci.bodyTwo->m_invMass * glm::dot(ci.contactNormal, cj.contactNormal);
					A_ij += glm::dot(rBNi, ci.bodyTwo->m_invInertia * rBNj);
				}
			}
		}


	}

	void ComputePreImpulseVelocity(const std::vector<Collisions::Contact>& contacts, std::vector<float>& ddot)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Contact ci = contacts[i];
			
			glm::vec3 rAi = ci.contactPoint - ci.bodyOne->m_position;
			glm::vec3 rBi = ci.contactPoint - ci.bodyTwo->m_position;
			glm::vec3 velA = ci.bodyOne->m_velocity + glm::cross(ci.bodyOne->m_angularVelocity, rAi);
			glm::vec3 velB = ci.bodyTwo->m_velocity + glm::cross(ci.bodyTwo->m_angularVelocity, rBi);
			ddot[i] = glm::dot(ci.contactNormal, velA - velB);
		}

	}

	void ComputeRestingContactVector(const std::vector<Collisions::Contact>& contacts, std::vector<float>& b)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Contact ci = contacts[i];
			Rigidbody* A = ci.bodyOne;
			Rigidbody* B = ci.bodyTwo;

			// Body one terms.
			glm::vec3 rAi = ci.contactPoint - A->m_position;
			glm::vec3 wAxrAi = glm::cross(A->m_angularVelocity, rAi);
			glm::vec3 At1 = A->m_invMass * A->m_externalForce;
			glm::vec3 At2 = glm::cross(A->m_invInertia * (A->m_externalTorque + glm::cross(A->m_angularMomentum, A->m_angularVelocity)), rAi);
			glm::vec3 At3 = glm::cross(A->m_angularVelocity, wAxrAi);
			glm::vec3 At4 = A->m_velocity + wAxrAi;

			// Body two terms.
			glm::vec3 rBi = ci.contactPoint - B->m_position;
			glm::vec3 wBxrBi = glm::cross(B->m_angularVelocity, rBi);
			glm::vec3 Bt1 = B->m_invMass * B->m_externalForce;
			glm::vec3 Bt2 = glm::cross(B->m_invInertia * (B->m_externalTorque + glm::cross(B->m_angularMomentum, B->m_angularVelocity)), rBi);
			glm::vec3 Bt3 = glm::cross(B->m_angularVelocity, wBxrBi);
			glm::vec3 Bt4 = B->m_velocity + wBxrBi;

			// Compute derivative of contact normal.
			glm::vec3 Ndot;
			if (ci.isVFContact) {
				Ndot = glm::cross(B->m_angularVelocity, ci.contactNormal);
			}
			else {
				glm::vec3 EdgeOnedot = glm::cross(A->m_angularVelocity, ci.edgeOne);
				glm::vec3 EdgeTwodot = glm::cross(B->m_angularVelocity, ci.edgeTwo);
				glm::vec3 U = glm::cross(ci.edgeOne, EdgeTwodot) + glm::cross(EdgeOnedot, ci.edgeTwo);
				Ndot = (U - glm::dot(U, ci.contactNormal)) * ci.contactNormal / glm::length(glm::cross(ci.edgeOne, ci.edgeTwo));
			}

			// Compute b vector element.
			b[i] = glm::dot(ci.contactNormal, At1 + At2 + At3 - Bt1 - Bt2 - Bt3) + (2.f * glm::dot(Ndot, At4 - Bt4));
		}
	}

	void DoImpulse(const std::vector<Collisions::Contact>& contacts, std::vector<float>& f)
	{
		for (int i = 0; i < contacts.size(); i++) {
			Collisions::Contact ci = contacts[i];
			Rigidbody* A = ci.bodyOne;
			Rigidbody* B = ci.bodyTwo;

			// Update momentum.
			glm::vec3 impulse = f[i] * ci.contactNormal;
			A->m_momentum += impulse;
			B->m_momentum -= impulse;
			A->m_angularMomentum += glm::cross(ci.contactPoint - A->m_position, impulse);
			B->m_angularMomentum -= glm::cross(ci.contactPoint - B->m_position, impulse);

			// Update velocity.
			A->m_velocity = A->m_invMass * A->m_momentum;
			B->m_velocity = B->m_invMass * B->m_momentum;
			A->m_angularVelocity = A->m_invInertia * A->m_angularMomentum;
			B->m_angularVelocity = B->m_invInertia * B->m_angularMomentum;
		}
	}

	void DoMotion(double t, double dt, const std::vector<Collisions::Contact>& contacts, std::vector<float> g) {
		for (int i = 0; i < contacts.size(); i++) {
			Contact ci = contacts[i];
			glm::vec3 resting = g[i] * ci.contactNormal;
			ci.bodyOne->AppendInternalForce(resting);
			ci.bodyOne->AppendInternalTorque(glm::cross(ci.contactPoint - ci.bodyOne->m_position, resting));
			ci.bodyTwo->AppendInternalForce(-resting);
			ci.bodyTwo->AppendInternalTorque(-glm::cross(ci.contactPoint - ci.bodyTwo->m_position, resting));
		}

		// Pulling this outside of the function.
		//for (int i = 0; i < rigidbodies.size(); i++) {
		//	rigidbodies[i].Update(t, dt);
		//}
	}

	void Minimize(const std::vector<float>& Avector, const std::vector<float>& dneg, std::vector<float>& dpos, std::vector<float>& f)
	{
		// Our general matrix size. We will be using 3 times this for most matrices (due to two conditions).
		int size = dneg.size();
		gte::GMatrix<float> A(size, size);
		gte::GMatrix<float> M(size * 3, size * 3);
		gte::GVector<float> q, b, c; 
		q = gte::GVector<float>(size * 3);
		b = c = gte::GVector<float>(size);

		// Generate the b and c vectors.
		for (int i = 0; i < size; i++) {
			if (dneg[i] >= 0.f)
				b[i] = 0.f;
			else
				b[i] = 2.f * dneg[i];

			c[i] = abs(dneg[i]);
		}

		// Create a matrix for A.
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				A(i, j) = Avector[(i * size) + j];
			}
		}

		// Fill the S matrix: S = A^T * A
		// Fill the M matrix:
		// { 2s -A  A
		//    A  0  0
		//   -A  0  0 }
		gte::GMatrix<float> S = gte::MultiplyATB(A, A);
		for (int i = 0; i < size; i++) {
			for (int j = 0; j < size; j++) {
				M(i, j) = 2.f * S(i, j);
				M(i + size, j) = -1.f * A(i, j);
				M(i + 2 * size, j) = A(i, j);
				M(i, j + size) = A(i, j);
				M(i, j + 2 * size) = -1.f * A(i, j);
			}
		}

		// Fill the q vector:
		// { 2b^TA
		//       b
		//     c-b }
		gte::GVector<float> double_bTA = 2.f * b * A;
		for (int i = 0; i < size; i++) {
			q[i] = double_bTA[i];
			q[i + size] = b[i];
			q[i + size * 2] = c[i] - b[i];
		}

		// Move data from GVectors and GMatrices into std::vectors.
		std::vector<float> MVector, QVector, ZVector, WVector;
		MVector = std::vector<float>(9 * size * size);
		QVector = ZVector = WVector = std::vector<float>(3 * size);
		std::copy_n(&M[0], 9 * size * size, MVector.begin());
		std::copy_n(&q[0], 3 * size, QVector.begin());
		
		gte::LCPSolver<float> lcpSolver = gte::LCPSolver<float>(3 * size);
		std::shared_ptr<gte::LCPSolver<float>::Result> result = std::make_shared<gte::LCPSolver<float>::Result>();
		if (lcpSolver.Solve(QVector, MVector, WVector, ZVector, result.get())) {
			// Get the output of f, calculate dpos.
			std::copy_n(ZVector.begin(), size, f.begin());
			// dpos = Af + dneg
			for (int i = 0; i < size; i++) {
				dpos[i] = dneg[i];
				for (int j = 0; j < size; j++) {
					dpos[i] += (A(i, j) * f[j]);
				}
			}

			if (*result == gte::LCPSolver<float>::Result::HAS_TRIVIAL_SOLUTION) {
			//	std::cout << "Trivial Result" << std::endl;
			}
			else {
			//	std::cout << "W: (" << WVector[0] << ", " << WVector[1] << ", " << WVector[2] << ")," << std::endl;
			//	std::cout << "Z: (" << ZVector[0] << ", " << ZVector[1] << ", " << ZVector[2] << ")" << std::endl;
			}
		}
		else {
			// There was no solution, return two zero vectors.
			std::fill(f.begin(), f.end(), 0);
			std::fill(dpos.begin(), dpos.end(), 0);
		}
	}


#pragma endregion Resting Contacts Collision Resolution Functions

}


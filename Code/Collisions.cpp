
#include "Collisions.h"
#include "glm/gtx/norm.hpp"	// Some "experimental" math functions (aka I'm too lazy to code square length myself, glm::length2)
#include "glm/gtx/normalize_dot.hpp" // For fastNormalize.
#include "GTE/Mathematics/LCPSolver.h"	// LCP solver :)
#include <iostream>

// If we detect penetration/non-penetration within this threshold, we have a contact.
#define COLLISION_THRESHOLD 0.01f
// Face contacts are usually better, so we apply a bias for it over edge edge contacts.
#define FACE_COLLISION_BIAS 0.02f

namespace Collisions {
	bool BoundingSphere(const Rigidbody& one, const Rigidbody& two)
	{
		float a = glm::length2(two.m_position - one.m_position);
		float b = pow(one.m_radius + two.m_radius, 2);
		if (glm::length2(two.m_position - one.m_position) > pow(one.m_radius + two.m_radius, 2))
			return false;
		return true;
	}
}

namespace Collisions {
#pragma region NEW Collision Detection Functions

	void SAT(Rigidbody& one, Rigidbody& two, ContactManifold& manifold)
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
		glm::vec3 oneEdgeDirection, oneEdgePoint, twoEdgeDirection, twoEdgePoint, collisionAxis;
		QueryEdgeDirections(one, two, CEdgeQueryPen, oneEdgeDirection, oneEdgePoint, twoEdgeDirection, twoEdgePoint, collisionAxis);
		if (CEdgeQueryPen > 0.f) return;	// separating axis found.

		// Hulls must overlap.
		bool blsFaceContactA = AFaceQueryPen + FACE_COLLISION_BIAS >= CEdgeQueryPen;
		bool blsFaceContactB = BFaceQueryPen + FACE_COLLISION_BIAS >= CEdgeQueryPen;
		if (blsFaceContactA && blsFaceContactB)
			CreateFaceContact(manifold, one, AFaceQueryPen, AFaceQueryPenIndex, two, BFaceQueryPen, BFaceQueryPenIndex);
		else
			CreateEdgeContact(manifold, one, two, CEdgeQueryPen, oneEdgeDirection, oneEdgePoint, twoEdgeDirection, twoEdgePoint, collisionAxis);

	}






#pragma endregion NEW Collision Detection Functions
}

// Helper functions.
namespace {
	// FIX THIS

	void QueryFaceDirections(const Rigidbody& one, const Rigidbody& two, float& largestPen, unsigned& largestPenIndex) {
		for (int index = 0; index < 6; ++index) {
			glm::vec3 planeNormalA = one.GetAxis(index);
			
			glm::vec3 planeLocalNormalA = one.GetLocalAxis(index);
			glm::vec3 localProjectedNormal = glm::dot(planeLocalNormalA, one.m_halfwidth) * planeLocalNormalA;
			if (planeLocalNormalA.x + planeLocalNormalA.y + planeLocalNormalA.z < 0)
				localProjectedNormal = -localProjectedNormal;
			glm::vec3 planeCenterA = one.m_orientation * localProjectedNormal + one.m_position;

			//glm::vec3 projectedHalfwidth = glm::sign(planeNormalA) * planeNormalA * glm::dot(planeNormalA, glm::abs(one.m_orientationMatrix * one.m_halfwidth));
			//glm::vec3 planeCenterA = glm::sign(planeNormalA) * planeNormalA * glm::dot(planeNormalA, glm::abs(one.m_orientationMatrix * one.m_halfwidth)) + one.m_position;
			//glm::vec3 planeCenterA = one.m_orientation * glm::sign(one.GetLocalAxis(index)) * (glm::abs(one.GetLocalAxis(index)) * glm::dot(glm::abs(one.GetLocalAxis(index)), one.m_halfwidth)) + one.m_position;
			glm::vec3 vertexB = two.GetSupport(-planeLocalNormalA);
			float distance = glm::dot(planeNormalA, (vertexB - planeCenterA));	// If distance is greater than zero, we found seperating axis.

			if (distance > 0.0f) {
				std::cout << "Object center: " << one.m_position[0] << ", " << one.m_position[1] << ", " << one.m_position[2] << std::endl;
				//std::cout << "Projected halfwidth: " << projectedHalfwidth[0] << ", " << projectedHalfwidth[1] << ", " << projectedHalfwidth[2] << std::endl;
				std::cout << "Plane normal: " << planeNormalA[0] << ", " << planeNormalA[1] << ", " << planeNormalA[2] << std::endl;
				std::cout << "Plane center: " << planeCenterA[0] << ", " << planeCenterA[1] << ", " << planeCenterA[2] << std::endl;
				std::cout << "Support point: " << vertexB[0] << ", " << vertexB[1] << ", " << vertexB[2] << std::endl;
				std::cout << "Distance: " << distance << std::endl << std::endl;
			}
			

			if (largestPen < distance) {
				largestPen = distance;
				largestPenIndex = index;
			}
		}
	}

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
	) 
	{
		// Greatly simplifying this function due to dealing with cuboids (usually would have to test every edge pair for convex hull).
		const glm::mat3 oneModel = (glm::mat3) one.GetModelMatrix();
		const glm::mat3 twoModel = (glm::mat3) two.GetModelMatrix();
		
		for (int i = 0; i < 3; ++i) {
			for (int j = 0; j < 3; ++j) {
				// Get the axis to project against.
				glm::vec3 axis = glm::cross(oneModel[i], twoModel[j]);
				if (glm::length2(axis) < 0.001f) continue;	// Skip near parallel edges.

				// Get all the center point of all four edges that point in the axis direction.
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
						oneEdgeDirection = oneModel[i];
						oneEdgePoint = edgePoint;
						twoEdgeDirection = twoModel[j];
						twoEdgePoint = vertexB;
						collisionAxis = axis;
					}
				}




			}
		}
	}

	void CreateFaceContact
	(
		Collisions::ContactManifold& manifold, 
		Rigidbody& one, 
		const float& aLargestPen,
		const unsigned& aLargestPenIndex,
		Rigidbody& two,
		const float& bLargestPen,
		const unsigned& bLargestPenIndex
	) 
	{
		Rigidbody* incidentBody; 
		Rigidbody* referenceBody;
		glm::vec3 referenceFaceNormal, incidentFaceNormal;
		
		auto GenerateManifold = [&]
		(
			Collisions::ContactManifold& manifold,
			Rigidbody& referenceBody,
			Rigidbody& incidentBody,
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
				float dot = glm::dot(testingFaceNormal, referenceFaceNormal);
				if (dot < smallestDot) {
					smallestDot = dot;
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
				incidentFacePoints.push_back((incidentFaceNormal - incidentModelMatrix[(i + 1) % 3] - incidentModelMatrix[(i + 2) % 3]) * incidentBody.m_halfwidth + incidentBody.m_position);
				incidentFacePoints.push_back((incidentFaceNormal - incidentModelMatrix[(i + 1) % 3] + incidentModelMatrix[(i + 2) % 3]) * incidentBody.m_halfwidth + incidentBody.m_position);
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
			
			// Once we have our set of points, move the set of contact points to the reference face.
			std::vector<glm::vec3> projectedPoints;
			glm::vec3 referencePlanePoint = referenceBody.m_position + (referenceFaceNormal * referenceBody.m_halfwidth);
			for (glm::vec3 point : incidentFacePoints) {
				// projPoint = p - (DOT(p-a, n) / DOT(n, n)) * n
				projectedPoints.push_back(point - glm::dot(point - referencePlanePoint, referenceFaceNormal) / glm::length2(referenceFaceNormal) * referenceFaceNormal);
			}

			// If we have more than four contact points, reduce to four.


			// Create the manifold.
			for (int i = 0; i < projectedPoints.size(); i++) {
				Collisions::Contact c;
				c.bodyOne = &incidentBody;
				c.bodyTwo = &referenceBody;
				c.contactNormal = referenceFaceNormal;
				c.contactPoint = projectedPoints[i];
				manifold.Points[i] = c;
				manifold.PointCount++;
			}
			manifold.Normal = referenceFaceNormal;



		};	// End of GenerateManifold function.

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
		Rigidbody& one,
		Rigidbody& two,
		const float& edgeLargestPen,
		glm::vec3& oneEdgeDirection,
		glm::vec3& oneEdgePoint,
		glm::vec3& twoEdgeDirection,
		glm::vec3& twoEdgePoint,
		glm::vec3& collisionAxis
	)
	{
		std::cout << "Edge-edge contact occured." << std::endl;

		// Find the closest point on each edge to the other edge.
		// The line between the closest points will be perpendicular to both edges.
		glm::vec3 closestDirection = glm::normalize(glm::cross(oneEdgeDirection, twoEdgeDirection));

		// Equations to find closest points sourced from math exchange post: 
		// https://math.stackexchange.com/questions/1414285/location-of-shortest-distance-between-two-skew-lines-in-3d
		glm::vec3 n_1 = glm::cross(oneEdgeDirection, closestDirection);
		glm::vec3 n_2 = glm::cross(twoEdgeDirection, closestDirection);
		glm::vec3 oneClosestPoint = oneEdgePoint + (glm::dot(twoEdgePoint - oneEdgePoint, n_2) / glm::dot(oneEdgeDirection, n_2)) * oneEdgeDirection;
		glm::vec3 twoClosestPoint = twoEdgePoint + (glm::dot(oneEdgePoint - twoEdgePoint, n_1) / glm::dot(twoEdgeDirection, n_1)) * twoEdgeDirection;

		// Find our "contact normal", which is the cross product between edges and

		// Contact point is the midpoint of these two closest points.
		Collisions::Contact c;
		c.contactPoint = (oneClosestPoint + twoClosestPoint) / 2.f;
		c.contactNormal = collisionAxis;
		c.bodyOne = &one;
		c.bodyTwo = &two;
		c.edgeOne = oneEdgeDirection;
		c.edgeTwo = twoEdgeDirection;
		c.isVFContact = false;

		manifold.Points[0] = c;
		manifold.PointCount = 1;
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


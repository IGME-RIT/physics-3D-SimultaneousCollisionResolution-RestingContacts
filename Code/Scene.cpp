/*
Copyright 2019
Original authors: Niko Procopi
Written under the supervision of David I. Schwartz, Ph.D., and
supported by a professional development seed grant from the B. Thomas
Golisano College of Computing & Information Sciences
(https://www.rit.edu/gccis) at the Rochester Institute of Technology.
This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or (at
your option) any later version.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
General Public License for more details.
<http://www.gnu.org/licenses/>.

Special Thanks to Exzap from Team Cemu,
he gave me advice on how to optimize Vulkan
graphics, he is working on a Wii U emulator
that utilizes Vulkan, see more at http://cemu.info
*/

#include "Scene.h"
#include <iostream>

// This function is found later in the file
// near the bottom under the destructor
Mesh* HitboxMesh(Mesh* base);

Scene::Scene()
{
	skyMesh = new Mesh("../../../Assets/skybox.3Dobj", false);

	// Load all the PNG files
	skyTex = new Texture("../../../Assets/skybox.png");


	// In this example, each entity has its own seperate
	// OBJ and PNG file. However, you can use the same
	// Model and Texture with multiple entities, without
	// needing to reload the same file multiple times


	// the skybox has a cube with a 2D PNG, not a cubemap
	skyEntity = new Entity();
	skyEntity->mesh = skyMesh;
	skyEntity->texture[0] = skyTex;
	skyEntity->CreateDescriptorSetBasic();

	// Create the enities for the scene.
	//cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(0.3f, 2, 1.1f), glm::vec3(0.5, 1.5, 2), glm::vec3(1, 1, 1)));
	//cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(0.0f, 1, 1.2f), glm::vec3(0.5, 0.5, 2), glm::vec3(1, 1, 1)));
	cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(-0.0f, 1, 0.f), glm::vec3(0.5, 0.5, 2), glm::vec3(1, 1, 1)));
	rigidbodies.push_back(std::make_shared<Rigidbody>(cuboids[0]->GetEntityPointers()));
	rigidbodies[0]->SetForceFunction(ForceFunctions::Gravity);
	rigidbodies[0]->SetTorqueFunction(ForceFunctions::NoTorque);

	// Static one.
	cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(0, -1, 0.0f), glm::vec3(2, 0.5, 0.5), glm::vec3(1, 1, 1)));
	rigidbodies.push_back(std::make_shared<Rigidbody>(cuboids[1]->GetEntityPointers(), false));
	rigidbodies[1]->SetForceFunction(ForceFunctions::NoForce);
	rigidbodies[1]->SetTorqueFunction(ForceFunctions::NoTorque);

	cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(0, 2.5f, 0), glm::vec3(0.7f, 0.7f, 0.7f), glm::vec3(1, 1, 1)));
	rigidbodies.push_back(std::make_shared<Rigidbody>(cuboids[2]->GetEntityPointers()));
	rigidbodies[2]->SetForceFunction(ForceFunctions::Gravity);
	rigidbodies[2]->SetTorqueFunction(ForceFunctions::NoTorque);
	 
	cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(0, 3.6f, 0), glm::vec3(0.3f, 0.3f, 0.3f), glm::vec3(1, 1, 1)));
	rigidbodies.push_back(std::make_shared<Rigidbody>(cuboids[3]->GetEntityPointers()));
	rigidbodies[3]->SetForceFunction(ForceFunctions::Gravity);
	rigidbodies[3]->SetTorqueFunction(ForceFunctions::NoTorque);

	// Get a time for when the scene starts.
	timePointSceneStart = std::chrono::steady_clock::now();
	timePointStartOfThisFrame = timePointSceneStart;
	timePerFrame = 1.f / frameRate;

}



void Scene::Update()
{
	// Update timings.
	timePointStartOfLastFrame = timePointStartOfThisFrame;
	timePointStartOfThisFrame = std::chrono::steady_clock::now();
	UpdateTimer(lastFramesTime, totalRunTime);	// update total run time and dt.
	//std::cout << lastFramesTime << std::endl;

	// The first (or second) time Update is called, there's a CPU bound lag spike
	// due to some rendering thing. We want to IGNORE that lag spike, as nothing's
	// actually rendered at that point. We use the boolean to make sure this only
	// runs due to the first render lag spike, not other lag spikes.
	if (firstUpdate && lastFramesTime > 0.15f) {

		// Update everything but physics.
		CheckKeyboardInput();
		UpdateCamera();
		UpdateText();
		for (auto rb : rigidbodies) {
			rb->Draw();
		}

		// Set the first update to false and return.
		firstUpdate = false;
		return;
	}



	// Consume all the render time since last frame.
	while (lastFramesTime > 0.0f) {

		// Calculate the amount of time to move forward by.
		float dt = glm::min(timePerFrame, lastFramesTime);
		
		
		// Update the simulation.
		CheckKeyboardInput();
		UpdateCamera();
		UpdateText();
		if (!isScenePaused) {
			UpdatePhysics(lastFramesTime, totalRunTime);
		}

		// Update timers.
		lastFramesTime -= dt;
		totalRunTime += dt;
	}

	for (auto rb : rigidbodies) {
		rb->Draw();
	}

}


void Scene::UpdatePhysics(float dt, float t) {

	// Set each rigidbody to update dt time (can be changed by collision detection).
	for (int i = 0; i < rigidbodies.size(); i++) {
		rigidbodies[i]->m_dt = dt;
		cuboids[i]->wireEntity->color = glm::vec3(0, 1, 0);
	}

	// Check collisions.
	for (int i = 0; i < rigidbodies.size(); i++) {
		for (int j = i + 1; j < rigidbodies.size(); j++) {				

			// If they pass the broad phase collision test, do SAT.
			if (Collisions::BoundingSphere(*rigidbodies[i].get(), *rigidbodies[j].get())) {

				// Check collisions using the new SAT method.
				std::shared_ptr<Collisions::ContactManifold> manifold = std::make_shared<Collisions::ContactManifold>();
				Collisions::SAT(*rigidbodies[i].get(), *rigidbodies[j].get(), *manifold.get());

				// If there's a collision, change the colors of the rigidbody outlines.
				if (manifold->PointCount > 0) {
					cuboids[i]->wireEntity->color = glm::vec3(1, 0, 0);
					cuboids[j]->wireEntity->color = glm::vec3(1, 0, 0);
				}
				
				// Add collision data to array.
				for (int i = 0; i < manifold->PointCount; i++) {
					contacts.push_back(manifold->Points[i]);
				}
			}
		}
	}

	// Collision response.
	int size = contacts.size();
	if (size > 0) {
		std::vector<float> A = std::vector<float>(size * size);	// This is a 2D matrix in the form of a vector.
		std::vector<float> preRelVel, postRelVel, impulseMag; 
		std::vector<float> restingB, relAcc, restingMag; 
		preRelVel = postRelVel = impulseMag = restingB = relAcc = restingMag = std::vector<float>(size);

		// Compute LCP Matrix.
		Collisions::ComputeLCPMatrix(contacts, A);
		//Collisions::ComputeAMatrix(contacts, A);

		// Guarantee no interpenetration by postRelVel >= 0.
		Collisions::ComputePreImpulseVelocity(contacts, preRelVel);
		//Collisions::Minimize(A, preRelVel, postRelVel, impulseMag);
		Collisions::ComputeImpulseResolution(A, preRelVel, postRelVel, impulseMag);
		//std::cout << std::endl;
		Collisions::DoImpulse(contacts, impulseMag);

		// Guarantee no interpenetration by relAcc >= 0.
		Collisions::ComputeLCPMatrix(contacts, A);
		Collisions::ComputeRestingContactVector(contacts, restingB);
		gte::LCPSolver<float> lcpSolver = gte::LCPSolver<float>(size);
		if (lcpSolver.Solve(restingB, A, relAcc, restingMag)) {
			//for (int i = 0; i < restingB.size(); i++) {
			//	std::cout << "[" << i << "]: " << restingB[i] << std::endl;
			//}
			//std::cout << std::endl;
			//for (int i = 0; i < relAcc.size(); i++) {
			//	std::cout << "[" << i << "]: " << relAcc[i] << std::endl;
			//}
			//std::cout << std::endl;
			//for (int i = 0; i < restingMag.size(); ++i) {
			//	if (restingMag[i] > 100.f || restingMag[i] < -100.f) {
			//		std::cout << "contact: " << contacts[i].isVFContact << std::endl;
			//		std::cout << "restingB: " << restingB[i] << std::endl;
			//	}
			//}
			Collisions::DoMotion(t, dt, contacts, restingMag);
		}
	}
	contacts.clear();

	// Update rigidbodies.
	for (std::shared_ptr<Rigidbody> rb : rigidbodies) {
		rb->Update(rb->m_dt, t);
	}
}


void Scene::CheckKeyboardInput() {

	// all key codes are here, https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	// Any of these can go into keys[]
	// including VK_ESCAPE, which was used in Main.cpp
	// VK does not mean "vulkan" in this case, VK means "Virtual Key",
	// it is part of Win32 and can be used in any API

	// to detect input with letters, you must use 
	// capital letters in the code

	// hitting the letter P on the keyboard
	// will pause the scene
	// hitting the letter O will resume.

	if (keys['A'])
	{
		angle += 0.01f;
	}
	if (keys['D']) {
		angle -= 0.01f;
	}

}

void Scene::UpdateCamera() {
	Demo* d = Demo::GetInstance();

	// create projection matrix
	// If this looks confusing, go back to
	// prepare_uniform_buffers and read those comments
	//d->projection_matrix = glm::ortho(-(float)d->width/2.f, (float)d->width/2.f, -(float)d->height/2.f, (float)d->height/2.f, 0.0f, 100.0f);
	//d->projection_matrix = glm::perspective(1.f * 3.14159f / 180.0f, (float)d->width / (float)d->height, 0.1f, 1000.0f);
	//d->projection_matrix = glm::perspective(90.f, (float)d->width / (float)d->height, 0.1f, 100.0f);
	d->projection_matrix = glm::perspective(45.0f * 3.14159f / 180.0f, (float)d->width / (float)d->height, 0.1f, 100.0f);
	/* Removed default spinning animation. */

	cameraPosition = { 10.0f * cos(angle), 1.0f, 10.0f * sin(angle) };

	// The position of the camera
	glm::vec3 focus = { 0, 0.0f, .0f };				// The position the camera is looking at
	glm::vec3 up = { 0.0f, 1.0f, 0.0 };			// The direction that faces up, which is the Y axis
	d->view_matrix = glm::lookAt(cameraPosition, focus, up);	// build the view matrix from thsi data

	// Notice the scale is small, yet the skybox is always drawn
	// behind everything else, this is how we know the Sky.Vert shader is 
	// working, because it sets depth to the maximum possible value
	skyEntity->pos = cameraPosition;
	skyEntity->scale = glm::vec3(1.0f);
}

void Scene::Draw()
{
	Demo* d = Demo::GetInstance();

	// clear the list of entities
	// that are being drawn. This list
	// is internally built and used by
	// Demo, and it controls how data
	// is sent to the GPU
	d->ResetEntityList();

	// Never try to draw the same entity twice. 
	// You can make 2 entities with the same Mesh,
	// but you can't draw the same entity twice.
	// The Demo system will actually block you from trying

	// Color pipeline	
	for (auto cuboidPtr : cuboids) {
		cuboidPtr->Draw(d);
	}

	// Bind the sky pipeline
	// Draw the sky, which will use the pipeSky pipeline
	d->ApplyPipelineSky();
	d->DrawEntity(skyEntity);

	// bind the hitbox pipeline
	//d->ApplyPipelineWire();
	//d->DrawEntity(catHitboxEntity);
	//d->DrawEntity(dogHitboxEntity);
}



char LetterToVirtualKey(char letter)
{
	return letter - 20;
}

void Scene::UpdateText() {



}

Scene::~Scene()
{
	// We delete our uniform buffer (which was on the CPU),
	// then we destroy all of our GPU buffers that were 
	// originally made from staging buffers

	delete skyEntity;
	delete skyMesh;
	delete skyTex;

}

void Scene::UpdateTimer(float& dt, float& t)
{
	dt = static_cast<float>((timePointStartOfThisFrame - timePointStartOfLastFrame).count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
	t += dt;
}


Mesh* HitboxMesh(Mesh* base)
{
	VertexWire* WireList = new VertexWire[19];

	// Min X side
	WireList[0].position[0] = base->min[0];
	WireList[0].position[1] = base->min[1];
	WireList[0].position[2] = base->min[2];

	WireList[1].position[0] = base->min[0];
	WireList[1].position[1] = base->min[1];
	WireList[1].position[2] = base->max[2];

	WireList[2].position[0] = base->min[0];
	WireList[2].position[1] = base->max[1];
	WireList[2].position[2] = base->max[2];

	WireList[3].position[0] = base->min[0];
	WireList[3].position[1] = base->max[1];
	WireList[3].position[2] = base->min[2];

	// Back to start
	WireList[4].position[0] = base->min[0];
	WireList[4].position[1] = base->min[1];
	WireList[4].position[2] = base->min[2];

	// Min Y side
	WireList[5].position[0] = base->min[0];
	WireList[5].position[1] = base->min[1];
	WireList[5].position[2] = base->max[2];

	WireList[6].position[0] = base->max[0];
	WireList[6].position[1] = base->min[1];
	WireList[6].position[2] = base->max[2];

	WireList[7].position[0] = base->max[0];
	WireList[7].position[1] = base->min[1];
	WireList[7].position[2] = base->min[2];

	// Back to start
	WireList[8].position[0] = base->min[0];
	WireList[8].position[1] = base->min[1];
	WireList[8].position[2] = base->min[2];

	// Min Z side
	WireList[9].position[0] = base->max[0];
	WireList[9].position[1] = base->min[1];
	WireList[9].position[2] = base->min[2];

	WireList[10].position[0] = base->max[0];
	WireList[10].position[1] = base->max[1];
	WireList[10].position[2] = base->min[2];

	WireList[11].position[0] = base->min[0];
	WireList[11].position[1] = base->max[1];
	WireList[11].position[2] = base->min[2];

	// Back to start
	WireList[12].position[0] = base->min[0];
	WireList[12].position[1] = base->min[1];
	WireList[12].position[2] = base->min[2];

	// Redraw existing line, start Max X
	WireList[13].position[0] = base->max[0];
	WireList[13].position[1] = base->min[1];
	WireList[13].position[2] = base->min[2];

	WireList[14].position[0] = base->max[0];
	WireList[14].position[1] = base->max[1];
	WireList[14].position[2] = base->min[2];

	WireList[15].position[0] = base->max[0];
	WireList[15].position[1] = base->max[1];
	WireList[15].position[2] = base->max[2];

	WireList[16].position[0] = base->max[0];
	WireList[16].position[1] = base->min[1];
	WireList[16].position[2] = base->max[2];

	// Redraw existing line, start Max Y
	WireList[17].position[0] = base->max[0];
	WireList[17].position[1] = base->max[1];
	WireList[17].position[2] = base->max[2];

	WireList[18].position[0] = base->min[0];
	WireList[18].position[1] = base->max[1];
	WireList[18].position[2] = base->max[2];

	return new Mesh(WireList, 19);
}
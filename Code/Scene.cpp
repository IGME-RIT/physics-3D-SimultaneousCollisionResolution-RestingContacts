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

	cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(0, 3, 0), glm::vec3(1, 0.5, 0.5), glm::vec3(1, 1, 1)));
	rigidbodies.push_back(std::make_shared<Rigidbody>(cuboids[0]->GetEntityPointers()));

	cuboids.push_back(std::make_shared<Cuboid>(glm::vec3(2, -2, 0.5f), glm::vec3(2, 0.5, 0.5), glm::vec3(1, 1, 1)));
	rigidbodies.push_back(std::make_shared<Rigidbody>(cuboids[1]->GetEntityPointers(), false));

	// Get a time for when the scene starts.
	timePointSceneStart = std::chrono::steady_clock::now();
	timePointStartOfThisFrame = timePointSceneStart;

	// Demo code for making 2d rectangle.
	//floor = std::make_unique<Shape>(std::vector<glm::vec3>{ glm::vec3(-10, -10, 0), glm::vec3(1, 1, 1),
	//														glm::vec3( 10, -10, 0), glm::vec3(1, 1, 1),
	//														glm::vec3( 10,  -3, 0), glm::vec3(1, 1, 1),
	//														glm::vec3(-10,  -3, 0), glm::vec3(1, 1, 1), });

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

// used to record time
float angle = 0.0f;
float dist = 10.0f;
float adjustZ = 3.0f;
bool noPressYet = true;
glm::vec3 cameraPosition;

char LetterToVirtualKey(char letter)
{
	return letter - 20;
}

void Scene::Update()
{
	// Update timings.
	timePointStartOfLastFrame = timePointStartOfThisFrame;
	timePointStartOfThisFrame = std::chrono::steady_clock::now();
	double dt = LastFrameTime();	// Time since start of last frame in seconds.
	//std::cout << "Time of last frame: " << dt << " seconds." << std::endl;

	CheckKeyboardInput();
	UpdateCamera();
	UpdateText();
	if (!isScenePaused) {
		UpdatePhysics(dt);
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

	if (keys['P'])
	{
		isScenePaused = true;
	}
	if (keys['O']) {
		isScenePaused = false;
	}


	if (keys[VK_UP])
	{
		// stop using default camera animation
		noPressYet = false;
	
		dist -= 0.05f;
	
		if (dist <= 1.0f)
			dist = 1.0f;
	}
	
	if (keys[VK_DOWN])
	{
		// stop using default camera animation
		noPressYet = false;
	
		dist += 0.05f;
	
		if (dist >= 9.0f)
			dist = 9.0f;
	}
	
	if (keys[VK_LEFT])
	{
		// stop using default camera animation
		noPressYet = false;
	
		angle -= 0.01f;
	}
	
	if (keys[VK_RIGHT])
	{
		// stop using default camera animation
		noPressYet = false;
	
		angle += 0.01f;
	}
}

void Scene::UpdatePhysics(double dt) {

	for (std::shared_ptr<Rigidbody> rb : rigidbodies) {
		rb->Update(dt);
	}

	// Check collisions.
	std::shared_ptr<Collisions::CollisionData> data = std::make_shared<Collisions::CollisionData>(8);
	Collisions::BoxBox(*rigidbodies[0].get(), *rigidbodies[1].get(), data.get());
	if (data->numOfContacts > 0) {
		cuboids[0]->wireEntity->color = glm::vec3(1, 0, 0); 
	}
}

void Scene::UpdateCamera() {
	Demo* d = Demo::GetInstance();

	// create projection matrix
	// If this looks confusing, go back to
	// prepare_uniform_buffers and read those comments
	d->projection_matrix = glm::perspective(45.0f * 3.14159f / 180.0f, (float)d->width / (float)d->height, 0.1f, 100.0f);
	float adjustZ = 0.0f;

	/* Removed default spinning animation. */

	cameraPosition = {
		dist * sin(angle),
		2.0f,
		dist * cos(angle) + adjustZ };

	// The position of the camera
	glm::vec3 focus = { 0, 0, 0 };				// The position the camera is looking at
	glm::vec3 up = { 0.0f, 1.0f, 0.0 };			// The direction that faces up, which is the Y axis
	d->view_matrix = glm::lookAt(cameraPosition, focus, up);	// build the view matrix from thsi data

	// Notice the scale is small, yet the skybox is always drawn
	// behind everything else, this is how we know the Sky.Vert shader is 
	// working, because it sets depth to the maximum possible value
	skyEntity->pos = cameraPosition;
	skyEntity->scale = glm::vec3(1.0f);
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

double Scene::LastFrameTime()
{
	return static_cast<double>((timePointStartOfThisFrame - timePointStartOfLastFrame).count()) * std::chrono::steady_clock::period::num / std::chrono::steady_clock::period::den;
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
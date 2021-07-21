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

// Some notes on this system:
//
// The code of the physics portion of this system is not optimized. It
// shouldn't be too hard to get it running much faster, but there's also
// an upper limit. More colliding rigidbodies in the scene causes the run
// time to increase quadratically. 
//
// Rigidbodies may clip into each other for a frame. This can be removed
// by checking if rigidbodies WILL collide in the next frame, and then
// stepping those rigidbodies by a different timestep than the rest of the
// scene such that they barely collide in the next frame, as opposed to 
// heavily colliding.
// 
// Written by Chris Hambacher, 2021.

#pragma once
#include "Demo.h"
#include "Shape.h"
#include "Cuboid.h"
#include "Rigidbody.h"
#include "Collisions.h"
#include <chrono>

class Scene
{
private:
	// Sky
	Mesh* skyMesh;
	Texture* skyTex;
	Entity* skyEntity;

	// Custom shapes.
	std::vector<std::shared_ptr<Cuboid>> cuboids;
	std::vector<std::shared_ptr<Rigidbody>> rigidbodies;

	// Used and populated in the UpdatePhysics function. Stores all of the contact points.
	std::vector<Collisions::Contact> contacts;

	// Timing variables
	bool isScenePaused = false;
	std::chrono::steady_clock::time_point timePointSceneStart;
	std::chrono::steady_clock::time_point timePointStartOfLastFrame;
	std::chrono::steady_clock::time_point timePointStartOfThisFrame;
	float lastFramesTime = 0;
	float totalRunTime = 0;

	// First update varibles and functions.
	bool firstUpdate = true;

	// Frame related variables.
	float frameRate = 120.f;
	float timePerFrame;

	// Camera.
	glm::vec3 cameraPosition;
	float angle = 0.0f;
	float adjustZ = 3.0f;


	// Functions called from Update
	void CheckKeyboardInput();
	void UpdatePhysics(float dt, float t);
	void UpdateText();
	void UpdateCamera();


public:
	bool* keys;
	Scene();
	void Draw();
	void Update();
	~Scene();

	void UpdateTimer(float& dt, float& t);
};


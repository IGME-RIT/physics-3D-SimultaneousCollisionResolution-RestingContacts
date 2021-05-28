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
	std::shared_ptr<Rigidbody> rb;

	// Timing variables
	bool isScenePaused = false;
	std::chrono::steady_clock::time_point timePointSceneStart;
	std::chrono::steady_clock::time_point timePointStartOfLastFrame;
	std::chrono::steady_clock::time_point timePointStartOfThisFrame;
	double lastFramesTime = 0;

	// Functions called from Update
	void CheckKeyboardInput();
	void UpdatePhysics(double dt);
	void UpdateText();
	void UpdateCamera();

public:
	bool* keys;
	Scene();
	void Draw();
	void Update();
	~Scene();

	double LastFrameTime();
};


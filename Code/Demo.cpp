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

#include "Demo.h"

#define _GNU_SOURCE
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <signal.h>
#include <vector>

#include "Helper.h"
#include "Main.h"
#include "Scene.h"

Scene* scene = nullptr;

// This boolean keeps track of how many times we have executed the
// "prepare()" function. If we have never used the function before
// then we know we are initializing the program for the first time,
// if the boolean is false, then we know the program has already
// been initialized
bool firstInit = true;

Demo* Demo::m_pInstance = nullptr;
Demo* Demo::GetInstance()
{
	if (m_pInstance == nullptr)   // Only allow one instance of class to be generated.
	{
		m_pInstance = new Demo();
	}

	return m_pInstance;
}

void Demo::prepare_console()
{
	// This line is commented out,
	// if you uncomment this "freopen" line,
	// it will redirect all text from the console
	// window into a text file. Then, nothing will
	// print to the console window, but it will all
	// be saved to a file

	//freopen("output.txt", "a+", stdout);

	// If you leave that line commented out ^^
	// then everything will print to the console
	// window, just like normal

	// Create Console Window
	
	// Allocate memory for the console
	AllocConsole();
	
	// Attatch the console to this process,
	// so that printf statements from this 
	// process go into the console
	AttachConsole(GetCurrentProcessId());

	// looks confusing, but we never need to change it,
	// basically this redirects all printf, scanf,
	// cout, and cin to go to the console, rather than
	// going where it goes by default, which is nowhere
	FILE *stream;
	freopen_s(&stream, "conin$", "r", stdin);
	freopen_s(&stream, "conout$", "w", stdout);
	freopen_s(&stream, "conout$", "w", stderr);
	SetConsoleTitle(TEXT("Console window"));

	// Adjust Console Window
	// This is completely optional

	// Get the console window
	HWND console = GetConsoleWindow();

	// Move the window to position (0, 0),
	// resize the window to be 640 x 360 (+40).
	// The +40 is to account for the title bar
	MoveWindow(console, 0, 0, 640, 360 + 40, TRUE);
}

void Demo::prepare_window()
{
	// Make the title of the screen "Loading"
	// while the program loads
	strncpy(name, "Loading...", APP_NAME_STR_LEN);

	WNDCLASSEX win_class = {};

	// Initialize the window class structure:
	// The most important detail here is that we
	// give it the WndProc function from main.cpp,
	// so that the function can handle our window.
	// Everything else here sets the icon, the title,
	// the cursor, thing like that
	win_class.cbSize = sizeof(WNDCLASSEX);
	win_class.style = CS_HREDRAW | CS_VREDRAW;
	win_class.lpfnWndProc = WndProc;
	win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
	win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
	win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	win_class.lpszMenuName = NULL;
	win_class.lpszClassName = name;
	win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);

	// RegisterClassEx will do waht it sounds like,
	// it registers the WNDCLASSEX structure, so that
	// we can use win_class to make a window

	// If the function fails to register win_class
	// then give an error
	if (!RegisterClassEx(&win_class))
	{
		// It didn't work, so try to give a useful error:
		printf("Unexpected error trying to start the application!\n");
		fflush(stdout);
		exit(1);
	}

	// Create window with the registered class:
	RECT wr = { 0, 0, width, height };
	AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);

	// We will now create our window
	// with the function "CreateWindowEx"
	// Win32 and DirectX 11 have some functions
	// with a huge ton of parameters. What 
	// Vulkan does for all functions, and what
	// DirectX 11 sometimes does, is organize
	// all parameters into a structure, and then
	// pass the structure as one parameter
	window = CreateWindowEx(0,
		name,            // class name
		name,            // app name
		WS_OVERLAPPEDWINDOW |  // window style
		WS_VISIBLE | WS_SYSMENU,

		// The position (0,0) is the top-left
		// corner of the screen, which is where
		// the command prompt is, and the command 
		// prompt is 640 pixels wide, so lets put
		// the Vulkan window right next to the console window
		640, 0,				 // (x,y) position
		wr.right - wr.left,  // width
		wr.bottom - wr.top,  // height
		NULL,                // handle to parent, no parent exists

		// This would give you "File" "Edit" "Help", etc
		NULL,                // handle to menu, no menu exists
		(HINSTANCE)0,		 // no hInstance
		NULL);               // no extra parameters

	// if we failed to make the window
	// then give an error that the window failed
	if (!window)
	{
		// It didn't work, so try to give a useful error:
		printf("Cannot create a window in which to draw!\n");
		fflush(stdout);
		exit(1);
	}

	// Window client area size must be at least 1 pixel high, to prevent crash.
	minsize.x = GetSystemMetrics(SM_CXMINTRACK);
	minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;
}

void Demo::prepare_instance()
{
	// The first thing we need to do, is choose the 
	// "layers" and "extensions" that we want to use in Vulkan
	// "Extensions" give us additional features, like ray tracing.
	// By default, Vulkan does not have support for any type of 
	// window: not iOS, Android, Windows, Linux, Mac, GLFW, 
	// or any type of Window, those are all available in different
	// extensions, which we will be using

	// "Layers" are what they sound like, they sit between
	// the driver and our code.
	// if we have no layers, then our code will speak
	// directly to the Vulkan driver, if we put a layer
	// between our code and the driver, the code will not be
	// as fast, but layers can still be very helpful, or even
	// simplify the programming process

	// In this case, we want the "Khronos Validation Layer".
	// Just like an HTML validation (which you may have used if you
	// made a website), the Vulkan validator will check our code for us,
	// and tell us if it thinks we've made any mistakes. This layer
	// can be disabled when development of a project is finished
	char *instance_validation_layer = "VK_LAYER_KHRONOS_validation";

	// our window is not minimized
	is_minimized = false;

	// We have one validation layer that we want to enable,
	// but we do not know if the layer is supported on the
	// computer that is running the software, so lets quickly
	// find out if it is supported

	// we create a boolean to see if we found the layer
	VkBool32 validation_found = 0;

	// only enable validation if the validate bool
	// is true. This boolean was set to true in init().
	// This is set to true or false by the programmer, it
	// does not depend on anything. It should be set to true
	// during development, and set to false when it is time 
	// to release the software
	if (validate)
	{
		// set the number of instance layers to zero
		// This is the number of total layers supported by 
		// our vulkan device, we set it to zero by default
		uint32_t total_instance_layers = 0;

		// What you are about to see, is something that Vulkan
		// does a lot, so please make sure you understand this 
		// before continuing. The function we are about to use is
		// vkEnumerateInstanceLayerProperties, it takes two parameters,
		// the first parameter is a pointer to a uint32_t number, and
		// the other parameter is a pointer to an array

		// if the array is set to null, 
		// then vkEnumerateInstanceLayerProperties will set the
		// uint32_t number equal to the total number of layers
		// that are available.

		// if the array is not set to null,
		// then vkEnumerateInstanceLayerProperties will fill the
		// array with elements, the number of elements will be equal
		// to the uint32_t number given in the first parameter

		// That means we will use vkEnumerateInstanceLayerProperties
		// two times, the first time will give us the number of
		// layers, and the second time will give us the array

		// call vkEnumerateInstanceLayerProperties for the 1st time,
		// get number of instance layers supported by the GPU
		vkEnumerateInstanceLayerProperties(&total_instance_layers, NULL);

		if (total_instance_layers > 0)
		{
			// Create an array that can hold the properties of every layer available
			// The properties of each layer (VkLayerProperties) will tell us 
			// the name of the layer, the version, and even a description of the layer
			VkLayerProperties* instance_layers = new VkLayerProperties[total_instance_layers];

			// call vkEnumerateInstanceLayerProperties for the 2nd time,
			// get the properties of all instance layers that are available
			vkEnumerateInstanceLayerProperties(&total_instance_layers, instance_layers);

			// If you did not understand the last three lines, please read the comments again.
			// This pattern WILL be used several times, maybe over 10 times, throughout the
			// course of the program

			// out of all the available layers,
			// check all the layers to see if the Khronos Validation Layer is supported.
			// We are checking each layer to see if the name of each layer is equal
			// to the name of the layer we want. If the layer we want is on the list
			// of supported layers, then we can use the validation layer, which we 
			// set at the beginning of the function
			for (uint32_t j = 0; j < total_instance_layers; j++)
			{
				// check to see if this layer has the same name as the one we want
				if (!strcmp(instance_validation_layer, instance_layers[j].layerName))
				{
					// we found the validation layer, woo!
					validation_found = 1;
					break;
				}
			}

			// we set the number of enabled layers to zero.
			// regardless if we found the validation layer in the 'for'
			// loop or not, we still have not enabled any layers yet
			enabled_layer_count = 0;

			// If we found the validation layer
			if (validation_found)
			{
				// then we have one layer that we can enable
				enabled_layer_count = 1;

				// add our validation layer to the list
				// of enabled layers. Technically this layer
				// is still not enabled yet, but it is guarranteed
				// that it can be enabled successfully, because we
				// just confirmed that it is supported. Don't worry
				// though, it will be enabled by the end of this function
				enabled_layers[0] = instance_validation_layer;
			}

			// we dont need the full list of instance layers
			// we can remove it now that we're done with it
			free(instance_layers);
		}

		// if we did not find a validation layer,
		// or, if we did not find any layers at all
		if (!validation_found || total_instance_layers <= 0)
		{
			// Give the user an error that we failed to find the validation layer.
			// If this error is found, you can fix it by disabling validate
			// in init()
			ERR_EXIT(
				"vkEnumerateInstanceLayerProperties failed to find required validation layer.\n\n"
				"Please look at the Getting Started guide for additional information.\n",
				"vkCreateInstance Failure");
		}
	}

	// Ok, we're done with layers, now it is time for
	// Vulkan extensions. If you are not sure what extensions
	// are, or why we need them, read the comments at the top of
	// this function

	// We need two extensions. One of them is for the "surface"
	// of Vulkan, which allows us to send rendered images to windows.
	// Without this extension, we can only render to textures or screenshots
	// This extension is generic, it lets us render to a "surface",
	// but does not connect the "surface" to a window

	// and the other extension is for the type of window
	// that we want to connect our surface to. This will be different
	// for every platform we program on: Windows, Linux, Mac, iOS, Android, etc.
	// Sometimes there are multiple surface extensions for one platform,
	// like how Windows has Win32 and GLFW, while Linux has Qt, xlib, and GLFW

	// set a boolean to see if we found the extension that
	// lets us render to a surface
	VkBool32 surfaceExtFound = 0;

	// set a boolean to see if we found the extension that
	// lets us connect a surface to a window
	VkBool32 platformSurfaceExtFound = 0;

	// clear the list of extension names in our "demo" structure
	memset(extension_names, 0, sizeof(extension_names));

	// set the number of instance extensions to zero
	// This is the number of total extensions supported by 
	// our vulkan device, we set it to zero by default
	uint32_t instance_extension_count = 0;

	// We are about to see the same pattern that we saw with 
	// vkEnumerateInstanceLayerProperties, where we will call
	// vkEnumerateInstanceExtensionProperties two times. The 
	// first time will give us the number of extensions,
	// and the second time will give us the array.
	// If this pattern does not make sense, please scroll up for
	// a detailed description of how this works with 
	// vkEnumerateInstanceLayerProperties

	// call the function for the 1st time, with the array set to NULL,
	// to get the number of extensions available
	vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, NULL);

	if (instance_extension_count > 0)
	{
		// create an array of extension properties, so that we can see the properties
		// of all the extensions that are supported. These properties hold the name
		// and version of each extension available
		VkExtensionProperties* instance_extensions = new VkExtensionProperties[instance_extension_count];

		// call this function for the 2nd time, passing the array, so that 
		// the array can be filled with all the extension properties available
		vkEnumerateInstanceExtensionProperties(NULL, &instance_extension_count, instance_extensions);

		// we set the number of extensions that are currently 
		// enabled to zero, because we haven't enabled anything yet
		enabled_extension_count = 0;

		// we check all of the extensions available 
		// to see if the extensions that we want are available to us
		for (uint32_t i = 0; i < instance_extension_count; i++)
		{
			// if this one particular extension is equal to VK_KHR_SURFACE_EXTENSION_NAME,
			// which we need for allowing us to render images to a "surface"
			if (!strcmp(VK_KHR_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
			{
				// then set the boolean to true
				// so that we know we found the extension
				surfaceExtFound = 1;

				// add the name of this extension to the list
				// of extensions that we can enable, and add
				// to the number of extensions that we are enabling
				extension_names[enabled_extension_count++] = VK_KHR_SURFACE_EXTENSION_NAME;
			}

			// if this one particular extension is equal to VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
			// which we need for allowing connect a "surface" to an HWND window with Win32 API
			if (!strcmp(VK_KHR_WIN32_SURFACE_EXTENSION_NAME, instance_extensions[i].extensionName))
			{
				// then set the boolean to true
				// so that we know we found the extension
				platformSurfaceExtFound = 1;

				// add the name of this extension to the list
				// of extensions that we can enable, and add
				// to the number of extensions that we are enabling
				extension_names[enabled_extension_count++] = VK_KHR_WIN32_SURFACE_EXTENSION_NAME;
			}
		}

		// we dont need the full list of instance extensions
		// we can remove it now that we're done with it
		free(instance_extensions);
	}

	// If we failed to find the extension that allows us
	// to send images to the "surface", then let the user
	// know that this failed
	if (!surfaceExtFound)
	{
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_SURFACE_EXTENSION_NAME
			" extension.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}

	// If we failed to find the extension that allows the
	// surface to send an image to the window, then let the user
	// know that this failed
	if (!platformSurfaceExtFound)
	{
		ERR_EXIT("vkEnumerateInstanceExtensionProperties failed to find the " VK_KHR_WIN32_SURFACE_EXTENSION_NAME
			" extension.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}

	// some tutorials will have VkApplicationInfo,
	// and then that will be put inside the 
	// structure of VkInstanceCreateInfo. However,
	// I personally believe that it is a waste of time.
	// All it does is hold the name of the app, the version,
	// the name of the engine, and the version, it literally
	// does not do or change anything in the program, so I 
	// just ignore it

	// It is time to talk about another pattern that 
	// will be used in all over the Vulkan program.

	// When we create the structure VkInstanceCreateInfo,
	// we will give it an "sType" that is ALWAYS equal 
	// to VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO.

	// Some people wonder "what is the point of having the
	// ability to change sType if it should always be the 
	// same anyways?"

	// Here is why: Eventually, we will take our object
	// of VkInstanceCreateInfo, which in this case is 
	// called "inst_info", and we will pass it to the
	// function "vkCreateInstance". When we do that,
	// we will give it a pointer to inst_info.

	// vkCreateInstance will check to see if the
	// first four bytes of the pointer is equal to 
	// VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO. If it
	// is equal, then that means we passed the correct
	// pointer to the function, which is "inst_info".
	// If vkCreateInstance does not see "VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO",
	// then that means the programmer might have made
	// a mistake, by giving the wrong poniter to the
	// vkCreateInstance function, and then the validation
	// layer will print an error in the command prompt window
	// that we made a mistake.

	// Therefore, the reason why every Vk---CreateInfo
	// structure has sType, is to help us make sure that
	// we are not making mistakes. If you do not understand this
	// then please read the commments again, this will
	// be a pattern that is seen throughout the entire tutorial

	// Also, the reason why Vulkan has structures for "create Info",
	// is so that we can pass the structure as one parameter,
	// rather than passing all of these variables as seperate
	// parameters. Some DirectX 11 functions like D3DCreateDeviceAndSwapchain,
	// have 10 parameters, and Vulkan puts the parameters into a structure,
	// to make the code look more organized

	// -----------------------------------------------------------------------
	// -----------------------------------------------------------------------
	// -----------------------------------------------------------------------

	// We want vulkan 1.1+ so that we can have negative veiwports.
	// Negative viewports are explained in void Demo::build_secondary_cmd().
	// The only variable that will impact the program is the API version

	// Vague version selection:		appInfo.apiVersion = VK_VERSION_1_2;
	// Specific version selection:	appInfo.apiVersion = VK_MAKE_VERSION(1, 2, 131);

	VkApplicationInfo app_info = {};
	app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	app_info.pApplicationName = "Vulkan Tutorial";
	app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.pEngineName = "RIT Engine";
	app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	app_info.apiVersion = VK_MAKE_VERSION(1, 2, 131);

	// We create an "instance" of Vulkan, so that we can start
	// using vulkan features, this allows us to use Vulkan commands
	// that the driver provides.

	// create our InstanceCreateInfo, with the necessary sType,
	// the number of layers we want to enable, the array of layers
	// that we want to enable, the number of extensions we want to enable,
	// and the array of extensions that we want to enable.
	VkInstanceCreateInfo inst_info = {};
	inst_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	inst_info.pApplicationInfo = &app_info;
	inst_info.enabledLayerCount = enabled_layer_count;
	inst_info.ppEnabledLayerNames = (const char *const *)enabled_layers;
	inst_info.enabledExtensionCount = enabled_extension_count;
	inst_info.ppEnabledExtensionNames = (const char *const *)extension_names;

	// Attempt to create a Vulkan Instance with the information provided.
	// We take the value that this returns, so that we can see if the
	// instance was created correctly
	VkResult err = vkCreateInstance(&inst_info, NULL, &inst);

	// If the function returns a value of -9,
	// then the driver is not compatible with Vulkan
	if (err == VK_ERROR_INCOMPATIBLE_DRIVER)
	{
		ERR_EXIT(
			"Cannot find a compatible Vulkan installable client driver (ICD).\n\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}

	// if the function returns a value of -7,
	// then there is an extension that is not supported
	// that we tried to enable. This will probably never
	// happen since we wnt through the necessary checks
	// earlier in the function
	else if (err == VK_ERROR_EXTENSION_NOT_PRESENT)
	{
		ERR_EXIT(
			"Cannot find a specified extension library.\n"
			"Make sure your layers path is set appropriately.\n",
			"vkCreateInstance Failure");
	}

	// if any other value is returned that is not zero,
	// then we got an unknown error
	else if (err)
	{
		ERR_EXIT(
			"vkCreateInstance failed.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}
}

void Demo::prepare_physical_device()
{
	// Now that we have an instance of Vulkan, we need
	// to determine how many GPUs are available that
	// can use Vulkan. We will be following the same
	// pattern as before: get the number of elements,
	// make an array, give the array to a function, and
	// the function will fill it with data

	// set the number of GPUs available to zero
	uint32_t gpu_count = 0;

	// In Vulkan, a PhysicalDevice holds all the information
	// about a GPU that is in a computer: how much memory it has,
	// what features it supports, the name of the GPU, the
	// company that made the GPU, the amount of processors,
	// literally everything.

	// HOWEVER, the PhysicalDevice only holds information
	// about the GPU, we cannot use PhysicalDevice to 
	// give commands to the GPU. For that, we need to 
	// create a "Device" from the same GPU. The first time
	// we do this, it will be confusing, but it will 
	// eventually make sense.

	// use the function vkEnumeratePhysicalDevices to get
	// the number of GPUs that are present in the computer.
	// we set the array of VkPhysicalDevice to NULL, so that
	// the function knows that it is looking for the number 
	// of GPUs
	vkEnumeratePhysicalDevices(inst, &gpu_count, NULL);

	// if we found a GPU
	if (gpu_count > 0)
	{
		// Create an array that will hold all PhysicalDevices in the computer
		VkPhysicalDevice* physical_devices = new VkPhysicalDevice[gpu_count];

		// Pass the array to this function, and then the array
		// will be filled with data that tells us about every GPU
		// in the computer that supports Vulkan
		vkEnumeratePhysicalDevices(inst, &gpu_count, physical_devices);

		// Each PhsyicalDevice be a dedicated graphics card, 
		// or an integraded graphics chip in a CPU, some devices
		// support graphics, some only support compute, there are
		// a lot of different types of GPUs out there that support Vulkan.

		// If someone wanted to, they could do a search to find which GPU
		// in the list most powerwful, or they could check for different
		// properties, but for this simple example, just take 
		// the first on the list, because that is usually best one

		// If you have multiple GPUs in your computer, maybe you have
		// both a dedicated GPU and a GPU integrated in the CPU,
		// set your dedicated GPU as your default graphics processor
		// in the Nvidia Control Panel, or AMD Catalayst, or Intel
		// Graphics Settings, and then that one will be the first
		// that shows up in the array
		gpu = physical_devices[0];

		// we do not need all of the devices anymore, we have
		// the one that we want
		free(physical_devices);

		// This gives us the name of the GPU, the number of processors
		// on the GPU, the amount of memory, the company that made the
		// GPU, everything there is to know
		vkGetPhysicalDeviceFeatures(gpu, &supportedFeatures);

		// We want to get properteis from the physical device.
		// This is completely option, I did it because I feel like it
		VkPhysicalDeviceProperties props;

		// This gives us the name of the GPU, the number of processors
		// on the GPU, the amount of memory, the company that made the
		// GPU, everything there is to know
		vkGetPhysicalDeviceProperties(gpu, &props);

		// set the title of the window to the name of the GPU,
		// so that we know we are using the GPU that we want to use
		SetWindowText(window, props.deviceName);
	}

	// If no GPUs were found, then 
	// give an error to the user
	else
	{
		ERR_EXIT(
			"vkEnumeratePhysicalDevices reported zero accessible devices.\n\n"
			"Do you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkEnumeratePhysicalDevices Failure");
	}

	// When we created the instance, we chose to use some extensions of the instance,
	// such as the SURFACE and WIN32 extensions. Now, we will enable extensions from
	// the PhysicalDevice. These extensions will help us with creating the "swapchain".

	// If a student does not know what a swapchain is, don't worry, the "swapchain" will
	// be fully explained in the prepare_swapchain() function, explaining what it is 
	// and why we need it. For now, here is how we enable the swapchain extension.

	// by default, we have not found the swapchain extension (yet)
	VkBool32 swapchainExtFound = 0;

	// Set the number of enabled_extensions to zero,
	// and clear the list of extension names. These
	// are the same variables we used for the instance,
	// but the instance doesn't need these variables now,
	// so we will re-use them for a similar purpose
	enabled_extension_count = 0;
	memset(extension_names, 0, sizeof(extension_names));

	// By default, there are zero device extensions that exist (that we know of)
	uint32_t device_extension_count = 0;

	// call this function to find out how many device extensions are available
	vkEnumerateDeviceExtensionProperties(gpu, NULL, &device_extension_count, NULL);

	// if there are more than zero extensions available,
	// then look for the extension that we want
	if (device_extension_count > 0)
	{
		// first we make a list that can hold all of the device's extensions
		VkExtensionProperties* device_extensions = new VkExtensionProperties[device_extension_count];

		// then we call the same function again to get the list of all extensions that are available
		vkEnumerateDeviceExtensionProperties(gpu, NULL, &device_extension_count, device_extensions);

		// we search through the list to look for the extension that we want,
		// which in this case, is the swapchain extension
		for (uint32_t i = 0; i < device_extension_count; i++)
		{
			// if we find the swapchain extension on the list of supported extensions
			if (!strcmp(VK_KHR_SWAPCHAIN_EXTENSION_NAME, device_extensions[i].extensionName))
			{
				// set this boolean so we know that we found it
				swapchainExtFound = 1;

				// add the swapchain to our list of extensions,
				// and increment the counter for the number of extensions
				extension_names[enabled_extension_count++] = VK_KHR_SWAPCHAIN_EXTENSION_NAME;
			}
		}

		// we do not need the list of extensions anymore,
		// now that we have already searched through it,
		// so we can delete it now
		free(device_extensions);
	}

	// if the swapchain was not found, then give an error and let the
	// user know that the swapchain could not be found
	if (!swapchainExtFound)
	{
		ERR_EXIT("vkEnumerateDeviceExtensionProperties failed to find the " VK_KHR_SWAPCHAIN_EXTENSION_NAME
			" extension.\n\nDo you have a compatible Vulkan installable client driver (ICD) installed?\n"
			"Please look at the Getting Started guide for additional information.\n",
			"vkCreateInstance Failure");
	}
}

void Demo::prepare_instance_functionPointers()
{
	// Almost all functions in Vulkan are built-in to the SDK.
	// We can access Vulkan functions through a static link (.lib),
	// or a dynamic link (.dll). However, some Vulkan functions are
	// not in the SDK, but they are actually inside the Vulkan Driver
	// itself. To get access to functions that are in the driver,
	// we need to use the VkInstance to get a pointer to the functions.

	// To shorten the code, we use a macro function to help
	// The macro function GET_INSTANCE_PROC_ADDR is completely optional.
	// If a student has not learned macro functions, i suggest googling it.
	// This macro function never needs to be changed, so you can just leave it here

	// There are 6 function pointers that we need to get from the instance
	// Without this macro function, we would need to rewrite this over and over:
	//		fpGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)vkGetInstanceProcAddr(inst, "vkGetPhysicalDeviceSurfaceSupportKHR");
	// and then check to see if each function pointer is null, and then return an error message

	// However, because we have the macro, we only need to write
	// GET_INSTANCE_PROC_ADDR(inst, "name"); for each function pointer that we want

	#define GET_INSTANCE_PROC_ADDR(inst, entrypoint)															\
    {																											\
        fp##entrypoint = (PFN_vk##entrypoint)vkGetInstanceProcAddr(inst, "vk" #entrypoint);						\
        if (fp##entrypoint == NULL)																				\
		{																										\
            ERR_EXIT("vkGetInstanceProcAddr failed to find vk" #entrypoint, "vkGetInstanceProcAddr Failure");	\
        }																										\
    }

	// All of these functions will be used later on in the code,
	// and they will be thoroughly explained when it is time to use them.
	GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfaceSupportKHR);
	GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfaceCapabilitiesKHR);
	GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfaceFormatsKHR);
	GET_INSTANCE_PROC_ADDR(inst, GetPhysicalDeviceSurfacePresentModesKHR);
	GET_INSTANCE_PROC_ADDR(inst, GetSwapchainImagesKHR);
	GET_INSTANCE_PROC_ADDR(inst, GetDeviceProcAddr);
}

void Demo::prepare_surface()
{
	// Create a Surface:
	// The "surface" is an abstraction layer that sends
	// a finished image from Vulkan to the screen

	// There are lots of different windows that can be used
	// With Vulkan, there are Win32 windows, GLFW windows, 
	// there are different windows on iOS, Android, Linux, and Mac.

	// The Surface is a middle-man that takes an image that Vulkan
	// creates (the image we want on the screen), and then the 
	// surface puts it on the screen

	// Today, we will create a surface by using VkWin32SurfaceCreateInfoKHR,
	// because we are using a Win32 window, so we will give it the 
	// necessary sType, and we will give it our window that we just created
	VkWin32SurfaceCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	createInfo.hwnd = window;

	// We use the instance we made, and the createInfo (which has
	// our window) to create a surface.
	vkCreateWin32SurfaceKHR(inst, &createInfo, NULL, &surface);

	// Now that we have our surface, we need to find a format that
	// is supported by (both) the surface, and the GPU. If the 
	// GPU can only export R8-B8-G8 images, but the surface (which connects
	// to the window) can only read R16-G16-B16-A16 images, that's bad.

	// the number of formats that are suppported by (both) the GPU, and our surface,
	// will be set to zero by default
	uint32_t formatCount = 0;
	
	// Just like we've seen before, Vulkan has a pattern of calling the same
	// function twice. First time gets the number of elements, Second time
	// fills an array with elements

	// get the number of formats
	fpGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, NULL);

	// create an array of surfaces that can hold the list of formats
	VkSurfaceFormatKHR *surfFormats = new VkSurfaceFormatKHR[formatCount];

	// Get the list of formats
	fpGetPhysicalDeviceSurfaceFormatsKHR(gpu, surface, &formatCount, surfFormats);

	// If there is only one format in the list, and if thath one format
	// is UNDEFINED, then there is no format that is supported by both
	// GPU and Surface. That means, we have to take a random guess and
	// hope we get lucky, so lets go with R8-G8-B8-A8.
	if (formatCount == 1 && surfFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		format = VK_FORMAT_R8G8B8A8_UNORM;
	}

	// If we find a format that is supported by the GPU and Surface
	// then we choose which format we want to use
	else
	{
		// we use an assert statement to make sure
		// that we have at least one format in the list.
		// I have never seen this assertion fail, but its
		// good to have it just in case
		assert(formatCount >= 1);

		// Out of all the formats that support GPU and Surface,
		// it really does not matter which one you pick, so lets
		// just go with the first one. Just like when we had
		// a list of VkPhysicalDevice, just pick the first one
		format = surfFormats[0].format;
	}

	// Format holds the type of image format that
	// the monitor can draw, while color_space is the 
	// type of format that the monitor uses. They sound
	// similar, but think about how the colors of a 
	// computer monitor might look a little different 
	// than a living-room TV or a movie theater. The 
	// same image with the same "format" on multiple
	// screens can end up looking a little different.
	// color_space gives us the format of the monitor,
	// so that the image can be sent to the monitor correctly
	color_space = surfFormats[0].colorSpace;

	// We don't need our array of surface formats,
	// now that we have the data we want, so we 
	// delete the array
	free(surfFormats);
}

void Demo::prepare_device_queue()
{
	//		Here is an explaination of how we will handle queues

	// In Vulkan, we have something called VkQueue. This queue allows us
	// to submit commands to the graphics card. The handling of queues
	// can vary from tutorial to tutorial, so I will quickly explain the 
	// differences

	// In Khronos' textured cube sample, they have two queues, one for
	// command buffers, and one for sending images to the screen. This
	// is not a good habit to have, because AMD GPUs only support one queue,
	// and performance is usually better with one queue, than multiple.
	// The Khronos tutorial also does not use staging buffers correctly,
	// it makes one staging buffer for an array of textures, where one 
	// buffer overwrites the other, causing memory leaks (more on that later)

	// Before writing this tutorial, I spoke to a Vulkan expert, who goes
	// by the name of Exzap, he is the lead Vulkan programmer for the 
	// Cemu project (Wii U emulator). He says that if multiple queues were
	// used, then "any performance gain will be ruined by synchronization
	// overhead from using multiple queues", so we will stick to one queue.
	// In addition, the RPCS3 and Xenia projects also only use one queue

	//		How to make a queue

	// get number of queues on the GPU
	uint32_t queue_family_count;

	// we use NULL for the array of queues to get the count
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, NULL);
	assert(queue_family_count >= 1);

	// we need to find if this queue supports graphics commands.
	// Rather than putting graphics commands and surface 
	// submission on seperate queues, we will put them on one queue

	// make an array of queue properties
	VkQueueFamilyProperties *queue_props;

	// allocate it
	queue_props = new VkQueueFamilyProperties[queue_family_count];
	vkGetPhysicalDeviceQueueFamilyProperties(gpu, &queue_family_count, queue_props);

	// we need to get properties of each queue on the graphics device.
	// out of all the queues on the gpu, we need to know which ones support
	// presenting to the surface, and which ones don't, so lets make a list
	// that can hold booleans for all the queues
	VkBool32 *queueSupportsPresent = new VkBool32[queue_family_count];

	// Iterate over each queue to learn whether it supports presenting
	// to the type of surface that we have, which is Win32Surface
	for (uint32_t i = 0; i < queue_family_count; i++)
	{
		// This is one of the functions that are built-in to the Vulkan driver,
		// we got the pointer to this function earlier in the code
		fpGetPhysicalDeviceSurfaceSupportKHR(gpu, i, surface, &queueSupportsPresent[i]);
	}

	// Search for a graphics and a present queue in the array of queue
	// families, try to find one that supports both
	uint32_t queue_family_index = UINT32_MAX;

	// check the properties of all queues on the device
	for (uint32_t i = 0; i < queue_family_count; i++)
	{
		// if this queue has a property flag that supports
		// graphics, then we can use this queue as a graphics queue
		if ((queue_props[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) != 0)
		{
			// if we have not found a queue index for graphics yet
			if (queueSupportsPresent[i] == VK_TRUE)
			{
				// set the graphics index queue equal to this one
				queue_family_index = i;
				break;
			}
		}
	}

	// we're done checking for support, so we can
	// delete the array of booleans
	free(queueSupportsPresent);

	// we don't need queue properties either
	free(queue_props);

	// Generate error if could not find both a graphics and a present queue
	if (queue_family_index == UINT32_MAX)
	{
		ERR_EXIT("Could not find queue\n", "Swapchain Initialization Failure");
	}

	// queues will have no priorities
	float queue_priorities[1] = { 0.0 };

	// create a queue info
	// this will describe how many queues to make (just one)
	// and which family indices to make the queues at (just one)
	VkDeviceQueueCreateInfo queueInfo = {};
	queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueInfo.queueFamilyIndex = queue_family_index;
	queueInfo.queueCount = 1;
	queueInfo.pQueuePriorities = queue_priorities;

	// create the device info
	// this tells us how a device will be made,
	// with the extensions that we can enable,
	// and with our queueInfo
	VkDeviceCreateInfo deviceInfo = {};
	deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	deviceInfo.queueCreateInfoCount = 1;
	deviceInfo.pQueueCreateInfos = &queueInfo;
	deviceInfo.enabledExtensionCount = enabled_extension_count;
	deviceInfo.ppEnabledExtensionNames = (const char *const *)extension_names;

	// initialize list of features that we want enabled
	desiredFeatures = {};

	// request anisotropy, if it is supported by physical device.
	// if it is not supported, then it wont be requested
	// This is for mipmapping and anisotropic filtering
	desiredFeatures.samplerAnisotropy = supportedFeatures.samplerAnisotropy;

	// The device will be enabled with all of our desired features
	deviceInfo.pEnabledFeatures = &desiredFeatures;

	// This function is called vkCreateDevice, but it actually
	// creates the device, and the queues, at the same time.
	// This works because the queueInfo is inside the deviceInfo
	vkCreateDevice(gpu, &deviceInfo, NULL, &device);

	// now that the device is created, the queues must also be created as well.
	// This function does not create the queues, because VkCreateDevice created the queues,
	// so insteadm, this function gets the queue from the device
	vkGetDeviceQueue(device, queue_family_index, 0, &queue);
}

void Demo::prepare_device_functionPointers()
{
	// When we ran the function prepare_instance_functionPointers() earlier in the code,
	// it allowed us to use the VkInstance to get functions that are not found in the SDK's .lib or .dll files,
	// but are instead in the Vulkan driver. Now, rather than using the VkInstance to get functions,
	// we will get some functions that are in the driver, but are only accessible through the VkDevice

	// Previously, in prepare_instance_functionPointers(), we used vkGetInstanceProcAddr in the 
	// GET_INSTANCE_PROC_ADDR macro function to get function-pointers from the driver, via the VkInstance.

	// This time, we will use fpGetDeviceProcAddr in the GET_DEVICE_PROC_ADDR macro function to get
	// function-pointers from the VUlkan driver, via the VkDevice (our graphics card).
	
	// An important note is that the vkGetInstanceProcAddr function was built-in to the SDK,
	// while fpGetDeviceProcAddr itself is a function pointer, which we got earlier in
	// prepare_instance_functionPointers()

	#define GET_DEVICE_PROC_ADDR(dev, entrypoint)															\
    {																										\
        fp##entrypoint = (PFN_vk##entrypoint) fpGetDeviceProcAddr(dev, "vk" #entrypoint);					\
        if (fp##entrypoint == NULL)																			\
		{																									\
            ERR_EXIT("vkGetDeviceProcAddr failed to find vk" #entrypoint, "vkGetDeviceProcAddr Failure");	\
        }																									\
    }

	// All of these functions will be used later on in the code,
	// and they will be thoroughly explained when it is time to use them.
	GET_DEVICE_PROC_ADDR(device, CreateSwapchainKHR);
	GET_DEVICE_PROC_ADDR(device, DestroySwapchainKHR);
	GET_DEVICE_PROC_ADDR(device, GetSwapchainImagesKHR);
	GET_DEVICE_PROC_ADDR(device, AcquireNextImageKHR);
	GET_DEVICE_PROC_ADDR(device, QueuePresentKHR);
}

void Demo::prepare_synchronization()
{
	// Create semaphores to synchronize acquiring presentable buffers before
	// rendering and waiting for drawing to be complete before presenting
	VkSemaphoreCreateInfo semaphoreCreateInfo = {};
	semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

	// Create fences that we can use to throttle if we get too far
	// ahead of the image presentation
	VkFenceCreateInfo fenceInfo = {};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

	for (uint32_t i = 0; i < FRAME_LAG; i++)
	{
		vkCreateFence(device, &fenceInfo, NULL, &drawFences[i]);

		vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &image_acquired_semaphores[i]);
		vkCreateSemaphore(device, &semaphoreCreateInfo, NULL, &draw_complete_semaphores[i]);
	}
	
	// start our frame_index at zero,
	// because that's where arrays
	// always start counting
	frame_index = 0;
}

void Demo::prepare_swapchain()
{
	// This function will create a swapchain,
	// and all of the images in the swapchain

	//			If you are new to graphics APIs
	//			Here is an explaination of a swapchain:
	//			What it does, and why we need it
	//
	// Lets pretend that Vulkan can render to the screen, and only
	// to the screen. That would mean that Vulkan would have to draw each pixel
	// on the screen, and then delete every pixel on the screen, and then start
	// rendering the next image to the screen. This is slow.
	// With a swapchain, we can have two images. When we first initialize Vulkan,
	// both swapchain images will be blank. Then, Vulkan will draw an image to one
	// of the swapchain images. After that, the completed image can be sent to the
	// monitor, while the 2nd swapchain image is being rendered. Then, after the 1st
	// image is done drawing on the screen and after the 2nd image is fully rendered,
	// they swap, which is why it is called "swapchain". Then, the 2nd image is being
	// drawn on the screen, while VUlkan is rendering a new frame to the 1st swapchain
	// image. Some swapchains have 2 images, and some have 3 images.

	// If a swapchain is so important, some might wonder why it is only available in an
	// extension (as discussed in a previous function), rather than being a core feature 
	// of Vulkan. Truthfully, not all Vulkan softwares use a swapchain, and not all GPUs 
	// support swapchains. Some Vulkan GPUs don't support graphics at all, and those will
	// Vulkan only for Compute, but those GPUs are your typical gaming cards.

	// Swapchains are an abolute standard for the gaming industry. Even the 
	// PlayStation 1 had the ability to use a swapchain, so I can guarrantee that any
	// popular consumer-level graphics card will support the swapchain extension.

	// Before we start making a new swapchain,
	// make a backup of the current swapchain.
	
	// If the swapchain is currently NULL
	// (during first init), then that's fine, 
	// the oldSwapchain will be NULL too. Otherwise
	// thd oldSwapchain will be a valid backup.
	VkSwapchainKHR oldSwapchain = swapchain;

	// Part 1: Get the dimensions
	// =======================================

	// Check the surface capabilities and formats.
	// This tells us what the surface is capable of doing
	VkSurfaceCapabilitiesKHR surfCapabilities;
	fpGetPhysicalDeviceSurfaceCapabilitiesKHR(gpu, surface, &surfCapabilities);

	// this holds the dimensions of all the swapcahin images,
	// which we will set in a minute
	VkExtent2D swapchainExtent;

	// When we call fpGetPhysicalDeviceSurfaceCapabilitiesKHR, there are two
	// possible outcomes:
	//	1: width and height are both 0xFFFFFFFF, 
	//	2: width and height are both not 0xFFFFFFFF.
	// That means we only need to check width.

	// If width is 0xFFFFFFFF, that means the surface is undefined,
	// and we need to define the surface by giving it dimensions.
	// This happens if the window is too big or too small for Vulkan
	// to handle
	if (surfCapabilities.currentExtent.width == 0xFFFFFFFF)
	{
		// If the surface size is undefined, the size is set to the size
		// of the images requested, which we need to adjust to make it
		// fit within the minimum and maximum values that are supported by the surface
		swapchainExtent.width = width;
		swapchainExtent.height = height;

		// if we are trying to use a width is less than the smallest width that is 
		// supported by our surface, then set the width to the smallest width that
		// is supported by the surface. We clamp the values to what is supported
		if (swapchainExtent.width < surfCapabilities.minImageExtent.width)
			swapchainExtent.width = surfCapabilities.minImageExtent.width;

		// if we are trying to use a width is bigger than the biggest width that is 
		// supported by our surface, then set the width to the biggest width that
		// is supported by the surface. We clamp the values to what is supported
		else if (swapchainExtent.width > surfCapabilities.maxImageExtent.width)
				 swapchainExtent.width = surfCapabilities.maxImageExtent.width;

		// if we are trying to use a height is less than the smallest height that is 
		// supported by our surface, then set the height to the smallest height that
		// is supported by the surface. We clamp the values to what is supported
		if (swapchainExtent.height < surfCapabilities.minImageExtent.height)
			swapchainExtent.height = surfCapabilities.minImageExtent.height;

		// if we are trying to use a height is bigger than the biggest height that is 
		// supported by our surface, then set the height to the biggest height that
		// is supported by the surface. We clamp the values to what is supported
		else if (swapchainExtent.height > surfCapabilities.maxImageExtent.height)
				 swapchainExtent.height = surfCapabilities.maxImageExtent.height;
	}
	
	else 
	{
		// If the surface size is already pre-defined, that
		// means that when we called fpGetPhysicalDeviceSurfaceCapabilitiesKHR,
		// the surface found a width and height that it wants to use,
		// which may be the size of the window, or it may be (0, 0) 
		// while minimizing, it could be a number of different things.

		// The point is, we have dimensions that the surface wants us to use,
		// and we need to make a swapchain with those dimensions so that the
		// images that Vulkan renders can be compatible with the current state
		// of the surface. This function will be called every time we call
		// prepare(), not just during the first initialization. This will
		// allow us to resize the window whenever we want.

		// We set the swapchainExtent to the size that 
		// the surface wants us to use.
		swapchainExtent = surfCapabilities.currentExtent;

		// We set width and height to those dimensions provided
		width = swapchainExtent.width;
		height = swapchainExtent.height;
	}

	// if width and height are zero, then set the boolean
	// to let us know that the window is minimized, and then
	// return, do not continue creating swapchain images. There
	// is no point in making a swapchain if there are no pixels
	// to render
	if (width == 0 || height == 0)
	{
		is_minimized = true;
		return;
	}

	// if our window is not minimzed, then set the boolean so
	// we know that it is not minimized, and then we continue
	// through the function to make the swapchain
	else 
	{
		is_minimized = false;
	}

	// Part 2: Get the present mode
	//=======================================================

	// We need to find a "Present Mode" that the surface supports. A "Present mode"
	// tells the surface and the window how to handle the images that we give it.
	// There are only 4 or 5 present modes that exist. Here is a list of them:

	//		Here is an explaination of what other present modes do
	//		The one that we use in this tutorial is FIFO

	// FIFO stands for First-in, FIrst-out
	// The PRESENT_MODE_FIFO present mode will give us VSYNC, and it will 
	// prevent tearing, so it will wait until one image is fully drawn
	// on the screen, before we try to draw another image.

	// IMMEDIATE will start drawing one frame to the screen,
	// even if another image is in the middle of being drawn, causes tearing.
	// VK_PRESENT_MODE_IMMEDIATE_KHR is for applications that don't care about
	// tearing, or have some way of synchronizing their rendering with the
	// display.

	// MAILBOX_KHR will disable VSYNC, and uncap the 60fps limit,
	// or whatever the limit of the monitor is.
	// VK_PRESENT_MODE_MAILBOX_KHR may be useful for applications that
	// generally render a new presentable image every refresh cycle, but are
	// occasionally early.  In this case, the application wants the new image
	// to be displayed instead of the previously-queued-for-presentation image
	// that has not yet been displayed.

	// VK_PRESENT_MODE_FIFO_RELAXED_KHR is for applications that generally
	// render a new presentable image every refresh cycle, but are occasionally
	// late. In this case (perhaps because of stuttering/latency concerns),
	// the application wants the late image to be immediately displayed, even
	// though that may mean some tearing.

	// This is the number of present modes are supported 
	// by the surface and the GPU. It is zero by default
	uint32_t presentModeCount = 0;
	
	// we get the number of modes that are supported
	fpGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, NULL);

	// we make an array to hold the list of supported modes
	VkPresentModeKHR *presentModes = new VkPresentModeKHR[presentModeCount];

	// Finally, we get the list of supported modes,
	// we use a function pointer that we got from the instance,
	// this function is built-in to the driver, not the SDK.
	fpGetPhysicalDeviceSurfacePresentModesKHR(gpu, surface, &presentModeCount, presentModes);
	
	// currentPresentMode is a member of the Demo class,
	// it is the presentation mode that is currently active.
	// If this is our first time here (firstInit), then the 
	// mode will be NULL (0), because there is no current
	// mode. Otherwise, That currentPresentMode will have the value
	// that it was given the last time this function was called,
	// which is the mode that is currently active

	// The mode we want is FIFO, becuase it locks our frame-rate to 
	// 60fps (or the limit of the monitor) and prevents tearing of images.
	VkPresentModeKHR desiredPresentMode = VK_PRESENT_MODE_FIFO_KHR;

	// If the current present mode is not equal to the 
	// present mode that we want to use. This will probably
	// only happen during firstInit
	if (desiredPresentMode != currentPresentMode)
	{
		// check all of the present modes that are supported
		// by the GPu and the surface
		for (size_t i = 0; i < presentModeCount; ++i)
		{
			// if the mode we want to use is on the list
			// of supported modes
			if (presentModes[i] == desiredPresentMode)
			{
				// then assign current to desired, allowing
				// us to use the mode that we want to use
				currentPresentMode = desiredPresentMode;
				break;
			}
		}
	}

	// If current is stil not equal to desire,
	// Then it means that our desired mdoe is not 
	// supported by the surface or GPU, so we 
	// give an error that addresses the problem
	if (currentPresentMode != desiredPresentMode)
	{
		ERR_EXIT("Present mode specified is not supported\n", "Present mode unsupported");
	}

	// delete the list of modes, we found the one we want,
	// so we don't need the entire list anymore
	if (presentModes != NULL)
		free(presentModes);

	// Determine the number of VkImages to use in the swapchain.
	// Application desires to acquire 3 images at a time for triple buffering.
	// This allows one image to be sent to the screen, while another image is
	// being created, while another image is being deleted.
	// In my opinion, I don't think there's a reason for more than three
	uint32_t desiredNumOfSwapchainImages = 3;

	// If the number of images we want to use is less than
	// the minimum number of images that the surface supports
	if (desiredNumOfSwapchainImages < surfCapabilities.minImageCount)
	{
		// then change the number of images to the minimum that
		// the surface supports. This would only happen if we tried
		// to use 0 images, or 1 image, but 3 images will totally be
		// above the minimum.
		desiredNumOfSwapchainImages = surfCapabilities.minImageCount;
	}

	// If maxImageCount is 0, then that means we can have as many
	// images as we want, because max is set to zero when there is no limit.
	// If max is more than zero, then there is a limit that we need to obey
	if (surfCapabilities.maxImageCount > 0)
	{
		// if the amount of images that we want in the swapchain,
		// is more than the maximum number that is supported by the 
		// GPU and the surface
		if (desiredNumOfSwapchainImages > surfCapabilities.maxImageCount)
		{
			// Application must settle for fewer images than desired:
			desiredNumOfSwapchainImages = surfCapabilities.maxImageCount;
		}
	}

	// Part 3: Get Flags
	// ===================================================

	// This is a transformation flag, it tells the swapchain
	// how to scale, rotate, or move an image, before it is sent to the surface.
	VkSurfaceTransformFlagsKHR preTransform;

	// I know what students are going to think:
	// "Can't we just send an image to the screen without transforming the image?"
	// Yes, you can, but only if the Surface allows you to do that.
	// Some surfaces require transformations, and some don't.

	// we check to see if our surface supports IDENTITY_TRANSFORM.
	// This is exactly what it sounds like, it is just like any other
	// identity matrix, it has no transformation, which means that if 
	// our surface supports IDENTITY, then we do not need to scale,
	// rotate, or translate an image, before it goes from swapchain to surface
	if (surfCapabilities.supportedTransforms & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR)
	{
		// we set the pre-transformation flag (the transformation of an image
		// after it is rendered, and before it goes to the surface) to IDENTITY,
		// so that there is no transformation
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	}

	// If the surface does not support IDENTITY, then it must require some kind
	// of transformation to be done to it, before the image is sent to the screen
	else 
	{
		// set preTransform to the transformation that the surface requires
		preTransform = surfCapabilities.currentTransform;
	}

	// When our image is done being rendered, there are red pixels,
	// green pixels, blue pixels, and alpha pixels. We all know what
	// the red, green, and blue pixels will do, but we do not know what the
	// alpha pixels will do. Should they be used for transparency? Should
	// they be ignored?

	// We need to choose what the Alpha pixels will do

	// By default, we use VK_COMPOSITE_ALPHA_OPAQUE, which will
	// not do anything with the alpha channel, it will just ignore it
	VkCompositeAlphaFlagBitsKHR desiredAlphaFlag = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	// Here are a few modes that we can set to handle the alpha.
	// These modes are in priority for what we want, depending on 
	// what the GPU supports. If the GPU does not support the ability
	// to ignore alpha, then we try using alpha to modify the RGB
	// values, and if that is not supported, then Vulkan will inherit
	// alpha properties from the surface
	VkCompositeAlphaFlagBitsKHR compositeAlphaFlags[4] = {
		VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR,
		VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR,
	};

	// loop through all the possibilities of alpha flags
	for (uint32_t i = 0; i < ARRAY_SIZE(compositeAlphaFlags); i++)
	{
		// find the first flag that our sruface supports
		if (surfCapabilities.supportedCompositeAlpha & compositeAlphaFlags[i])
		{
			// set our desired flag to the one that
			// our surface supports
			desiredAlphaFlag = compositeAlphaFlags[i];

			// leave the loop, stop looking for flags,
			// because we found the one that we want
			break;
		}
	}

	// Part 4: Make the empty swapchain
	//====================================================

	// Lets take everything that we've made so far,
	// bundle it together, and make a swapchain with it

	VkSwapchainCreateInfoKHR swapchain_ci = {};
	swapchain_ci.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchain_ci.surface = surface;
	swapchain_ci.minImageCount = desiredNumOfSwapchainImages;
	swapchain_ci.imageFormat = format;
	swapchain_ci.imageColorSpace = color_space;
	swapchain_ci.imageExtent.width = swapchainExtent.width;
	swapchain_ci.imageExtent.height = swapchainExtent.height;
	swapchain_ci.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	swapchain_ci.preTransform = (VkSurfaceTransformFlagBitsKHR)preTransform;
	swapchain_ci.compositeAlpha = desiredAlphaFlag;
	swapchain_ci.imageArrayLayers = 1;
	swapchain_ci.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	swapchain_ci.presentMode = currentPresentMode;
	swapchain_ci.oldSwapchain = oldSwapchain;
	swapchain_ci.clipped = true;
	
	// We create the swapchain with a function pointer,
	// because this function is not in the SDK, it is in the VUlkan Driver,
	// and we get access to it by finding the pointer via the device
	// (we found the function pointers earlier in the code)

	// use the function pointer, the device, and the 
	// swapchain CreateInfo structure to create the swapchain
	fpCreateSwapchainKHR(device, &swapchain_ci, NULL, &swapchain);

	// If we just re-created an existing swapchain, we should destroy the old
	// swapchain. We know if an old swapchain exists by checking if it is NULL.
	// Note: destroying the swapchain also cleans up all its associated
	// presentable images once the platform is done with them.
	if (oldSwapchain != VK_NULL_HANDLE)
	{
		// destroy the old swapchain
		fpDestroySwapchainKHR(device, oldSwapchain, NULL);
	}

	// Part 5: Make the swapchain's images usable
	//=================================================================

	// Now that we've used the fpCreateSwapchainKHR function,
	// we now have a swapchain, and it is filled with images that
	// we can render to, but first we need to put these images into
	// a state that we can work with

	// get the amount of images that are in the swapchain
	fpGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, NULL);

	// make an array to hold the swapchain's images
	VkImage *swapchainImages = new VkImage[swapchainImageCount];

	// get the images that are in the swapchain that we just made
	fpGetSwapchainImagesKHR(device, swapchain, &swapchainImageCount, swapchainImages);

	// Our swapchain images are in the form of VkImage. While
	// they are in the form of VkImage, they cannot be directly
	// accessed by the CPU or GPU

	// The structure SwapchainImageResources is defined in Demo.h,
	// and we have an array in the Demo class that holds elements of 
	// SwapchainImageResources, which we will be using throughout
	// the rest of the tutorial, so make an array of SwapchainImageResources
	// so that we have resources for every swapchain image
	swapchain_image_resources = new SwapchainImageResources[swapchainImageCount];

	// The swapchain_image_resources array, basically makes it so each
	// swapchain iamge has its own image (itself), its own imageView
	// (which makes the image usable), its own frameBuffer (which allows
	// the imageView to be used in drawing), and its own command buffer,
	// which will be explained later

	// loop through all the swapchain images
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		// Create an image veiw
		// VkImageView will act as a wrapper for VkImage
		// which allows the data of VkImage to be accessible
		// by the CPU and GPU

		// One way to imagine it is:
		// VkImage is the a package in the Post Office,
		// but the package can't go anywhere without a stamp.
		// VkImageView is the stamp

		// We have to set a number of variables that 
		// describe the image that will be inside imageView
		// sType will always be VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO
		VkImageViewCreateInfo viewInfo = {};
		viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;

		// we have to give the type of image, this is a 2D
		// image. Sounds obvious, but it is possible to have
		// 3D images, or image arrays
		viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		
		// we have to include the format of the images
		// which we got from Surface
		viewInfo.format = format;

		// we hvae to label each of the color components
		viewInfo.components.r = VK_COMPONENT_SWIZZLE_R;
		viewInfo.components.g = VK_COMPONENT_SWIZZLE_G;
		viewInfo.components.b = VK_COMPONENT_SWIZZLE_B;
		viewInfo.components.a = VK_COMPONENT_SWIZZLE_A;

		// we have to mention that this is a color image,
		// there are other types of images that are used
		// for things other than color, like depth
		// but more information will be given about that later
		viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		// If you don't know what mipmaps are, or mipmap chains, 
		// don't worry about it. This image (the swapchain image),
		// does not have a mipmap chain, so the base level is 0,
		// and there is 1 level. Honestly just don't worry about it.
		viewInfo.subresourceRange.baseMipLevel = 0;
		viewInfo.subresourceRange.levelCount = 1;

		// Technically, in Vulkan's eyes, every image is an array
		// so we tell it that there is one image in the array,
		// and that the element of the array we want is 0,
		// which literally means "there is one image, so use
		// that one image"
		viewInfo.subresourceRange.baseArrayLayer = 0;
		viewInfo.subresourceRange.layerCount = 1;

		// set the imageView's image to the swapchain image
		viewInfo.image = swapchainImages[i];

		// Make a backup of the image, so each
		// member of the swapchain_image_resources has the 
		// original VkImage
		swapchain_image_resources[i].image = viewInfo.image;

		// Use the viewInfo to make an imageView, for each swapchain image,
		// in the array of swapchain_image_resources
		vkCreateImageView(device, &viewInfo, NULL, &swapchain_image_resources[i].view);

	}

	// if an array of swapchainImages exists delete the array.
	// Keep in mind, this is not the array of swapchain_image_resources,
	// this is just the array of VkImage. We can delete this now because
	// the images have been copied to the swapchain_image_resources array
	if (swapchainImages != NULL)
		free(swapchainImages);
}

void Demo::prepare_init_cmd()
{
	// This command buffer will be used to copy data from
	// the CPU to the GPU. We will finish this command buffer
	// after we have given it all of our copy commands

	VkCommandBufferAllocateInfo cmdInfo = {};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdInfo.commandPool = cmd_pool;
	cmdInfo.commandBufferCount = 1;

	vkAllocateCommandBuffers(device, &cmdInfo, &initCmd);
}

void Demo::begin_init_cmd()
{
	// This is the easiest function in the tutorial

	// make a structure for all parameters that we want
	// for the command buffer. In this case we don't want
	// any specific adjustments, but we will have more 
	// parameters for other command buffers later.
	// sType is required
	VkCommandBufferBeginInfo cmd_buf_info = {};
	cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Begin the command buffer, so all Vulkan commands
	// will go here until further notice. All commands
	// in this command buffer will be executed later 
	// in the prepare() function, before we start drawing
	// to the screen
	vkBeginCommandBuffer(initCmd, &cmd_buf_info);
}

void Demo::prepare_sampler()
{
	// A sampler is used by the graphics card to get each pixel from the texture. Raw
	// pixel data from a texture is not enough, the sampler gets the pixel out of the 
	// texture, depending on UV coordinates. We only need one sampler for the entire
	// program, regardless of how many textures we have. In extreme cases like
	// Call of Duty and Grand Theft Auto, they probably have 10 different samplers
	// (probably less) that are shared for all of their textures

	// Create information about our sampler
	// Give it the required sType
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;

	// This handles filtering of the texture.
	// If the pixels of the texture don't perfectly
	// allign with pixels on the screen, grab the
	// pixel of the texture that is nearest to the 
	// pixel on the screen
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;

	// This is for mipmaps, and we don't have mipmaps
	// so just ignore this for now. It grabs the nearest 
	// mipmap layer, but images with no mipmaps will have
	// all pixels on one layer, so just ignore it for now
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;

	// All images have UV values that range from 0 to 1.
	// If we go outside of this 0 to 1 range, then the texture
	// repeats. Our UVs wont go outside of the 0 to 1 range,
	// so you can ignore it. We can also use BORDER and CLAMP
	// which make pixels outside the 0 to 1 range the same as
	// the pixels on the edge of the texture, or they can all
	// default to black, it doesn't really matter for now
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;

	// Use anisotropic filtering with mipmaps
	if (supportedFeatures.samplerAnisotropy)
	{
		samplerInfo.anisotropyEnable = VK_TRUE;
		samplerInfo.maxAnisotropy = 16;
		samplerInfo.maxLod = 16;
	}

	// we don't need any compare operations, we just need pixels
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;

	// if we are using CLAMP for addressMode, then default 
	// the pixel color to WHITE
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;

	// create the sampler, based on the information we want
	vkCreateSampler(device, &samplerInfo, NULL, &sampler);
}

void Demo::prepare_descriptor_layout()
{
	// Each descriptorSetLayoutBinding will describe what type
	// of descriptor we will have at each binding in the shaders
	VkDescriptorSetLayoutBinding layout_bindings[3];
	memset(layout_bindings, 0, sizeof(VkDescriptorSetLayoutBinding) * 3);

	// What I like about the VkDescriptorSetLayoutBinding is that
	// it is structured similarly to an english sentence. For
	// example, look at the comment below, compared to the 
	// structure

	// In the Veretx Shader, at binding 0, we have 1 descriptor, which is a uniform buffer
	layout_bindings[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	layout_bindings[0].binding = 0;
	layout_bindings[0].descriptorCount = 1;
	layout_bindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	// That was easy enough, and it didn't require sType
	// Now we have to create a descriptor layout with our array
	// of descriptor layout bindings

	// descriptor layout with one binding
	// we give it the array, and we let the structure
	// know that there are 1 elements in the array.
	// sType is required
	VkDescriptorSetLayoutCreateInfo descriptor_layout = {};
	descriptor_layout.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptor_layout.bindingCount = 1;
	descriptor_layout.pBindings = layout_bindings;

	// create the descriptor layout with the information we provided
	vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, &desc_layout_color);

	// Wireframe Hitboxes

	// We add another binding for all the other pipelines
	// In the Fragment Shader, at binding 1, we have 1 descriptor, which is
	// a uniform buffer that holds a color
	layout_bindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layout_bindings[1].binding = 1;
	layout_bindings[1].descriptorCount = 1;
	layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;

	// descriptor layout with two bindings
	// we give it the array, and we let the structure
	// know that there are 2 elements in the array.
	// sType is required
	descriptor_layout.bindingCount = 2;

	// create the descriptor layout with the information we provided
	vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, &desc_layout_wire);

	// Basic objects
	// 3D objects with textures on them

	// All we have to do is change the fragment shader descriptor to
	// have an image sampler instead of a uniform buffer
	layout_bindings[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	// create the descriptor layout with the information we provided
	vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, &desc_layout_basic);

	// That's the end of the basic descriptor, now let's do the bumpy descriptor
	// that will be used for normal mapping

	// In the Fragment Shader, at binding 2, we have 1 descriptor, which is an image
	// This will be the normal map texture
	layout_bindings[2].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
	layout_bindings[2].binding = 2;
	layout_bindings[2].descriptorCount = 1;
	layout_bindings[2].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;

	// increment the counter
	descriptor_layout.bindingCount = 3;

	// create the descriptor layout with the information we provided
	vkCreateDescriptorSetLayout(device, &descriptor_layout, NULL, &desc_layout_bumpy);
}

void Demo::prepare_descriptor_pool()
{
	// There are two types of descriptors in the pool
	// Uniform Buffer Descriptors
	// Texture Descriptors
	// We need to keep count of how many there are of each
	VkDescriptorPoolSize type_counts[2];

	// descriptor count is the number of buffers (of each type)
	// that will be used in all pipelines for the entire program.
	// In this case, its one uniform buffer and one texture.

	// Changed in tutorial 18

		// Tutorials 1-17 set the maximum sizes to the perfect amount
		// of descriptors and sets that was needed for the tutorials,
		// no more and no less.

		// The reason we did that was to make sure that there were no
		// unintended buffers or leaks

		// At this point, the tutorials are proven to be stable, so we
		// can remove this restriction. By setting the "count" variables
		// higher, we can dynamically load more assets after the game 
		// initially loads, if we ever want to try it.

		// No additional memory will be allocated by increasing "count"
		// variables, so there is no need to worry about leaks

	// We can have a maximum of 512 Uniform Buffers
	// this can be increased at any time
	type_counts[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	type_counts[0].descriptorCount = 512;

	// We can have a maximum of 512 Uniform Buffers
	// this can be increased at any time
	type_counts[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	type_counts[1].descriptorCount = 512;

	// poolSizeCount is 2 
	// that is the number of elements in the type_counts array.
	// If we increase the number of descriptors, this number should
	// still be two. We would increase that number of we wanted
	// other types of descriptors, like Storage Buffers (compute)
	
	// Max Sets will store the number of dsecriptor sets that
	// we will have in the pool
	// We can have a maximum of 512 sets,
	// this can be increased at any time

	VkDescriptorPoolCreateInfo descriptor_pool = {};
	descriptor_pool.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptor_pool.maxSets = 512;
	descriptor_pool.poolSizeCount = 2;
	descriptor_pool.pPoolSizes = type_counts;

	// create descriptor pool, based on the information provided
	vkCreateDescriptorPool(device, &descriptor_pool, NULL, &desc_pool);
}

void Demo::prepare_depth_buffer()
{
	// The depth buffer holds the depth of each 
	// pixel, so that when the final image is being
	// created, the GPU will prioritize pixels with
	// the smallest depth. That way, polygons closer
	// to the camera will draw on top of polygons
	// that are farther from the camera.
	
	// This ^^ is set in pipelineState's
	// depthStencilState, which will be described
	// later. That is what allows us to choose
	// what the depth buffer is used for

	// The format of this image will be 16-bit.
	// There will be 16 bits that will store the depth 
	// of each pixel, which is plenty for this example.
	// If a scene has more depth, it can be expanded
	// to D32_UNORM
	const VkFormat depth_format = VK_FORMAT_D16_UNORM;

	// We create an image for the depth buffer.
	// This is our first time making an image by hand,
	// because the Swapchain's create-function generated
	// the images for us, and then we put the images
	// into an imageView. This is how an image is created
	// normally

	// The sType is always the same
	// This is a 2D image, not an array, not a skybox
	// we give it the format
	// we give it the width and height (same as screen size)
	// the depth is 1, but this depth is not the same as pixel depth.
	// There are 3D textures that have a width, height, and depth.
	// This is a 2D image, so we have a width, a height, and a depth of 1 pixel
	// the usage of this image is for the depth stencil
	VkImageCreateInfo image = {};
	image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	image.imageType = VK_IMAGE_TYPE_2D;
	image.format = depth_format;
	image.extent.width = width;
	image.extent.height = height;
	image.extent.depth = 1;
	image.mipLevels = 1;
	image.arrayLayers = 1;
	image.samples = VK_SAMPLE_COUNT_1_BIT;
	image.tiling = VK_IMAGE_TILING_OPTIMAL;
	image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;

	// We use our TextureGPU class to make a special type of buffer for the texture
	// that is on the GPU. Unlike the Vertex and Index buffers, we do not copy
	// a CPU buffer into this GPU image, because depth information is generated
	// by the GPU. The GPU will store data here, and read data here, no CPU involvement
	// is necessary. We tell the GPU to use this buffer in the render_pass function,
	// which will be explained later
	depthBufferGPU = new TextureGPU(
		device,
		width, height,
		memory_properties,
		image,
		VK_IMAGE_ASPECT_DEPTH_BIT);

	// we give it the format of depth that we want it to use,
	// which was set earlier in the function
	depthBufferGPU->format = depth_format;
}

void Demo::execute_init_cmd()
{
	// end the initialization command buffer.
	// all command buffers need to be ended
	// before submission, and now we are ready
	// to submit this list of commands, which should
	// copy buffers from CPU to GPU (vertex, index,
	// textures, etc).
	vkEndCommandBuffer(initCmd);

	// we create a VkSubmitInfo
	// All we do here, is provide information that
	// says there will be one command buffer, which
	// is the initialization command buffer.
	// no other information is needed right now

	// other members of VkSubmitInfo this structure include
	// semaphores to wait for, and semaphores to trigger,
	// allowing for certain command buffers to wait for 
	// other command buffers to finish, prior to execution
	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &initCmd;

	// we submit the VkSubmitInfo to the queue
	// so that the command buffer can be executed 
	// by the GPU.

	// we give it the fence, which is "closed" by default.
	// when the GPU finishes executing the command buffer,
	// then the GPU will "open" the fence.
	vkQueueSubmit(queue, 1, &submit_info, initFence);

	// after the queue is finished, we have to wait until command
	// buffer is finished executing, we will know it is done when
	// the "fence" tells us, with this function.

	// vkWaitForFences will pause the program while a fence is 
	// closed. 
	
	// Imageine in real life, 
	// standing in front of a physical fence,
	// the fence is closed, so you cannot continue walking.
	// When the fence opens, you continue walking. 
	
	// Our C++ code sees that the fence is closed, so it pauses.
	// When the fence is open, our code will contiue past this line
	vkWaitForFences(device, 1, &initFence, VK_TRUE, UINT64_MAX);

	// Now that we've gotten past that line ^^
	// that means the fence must be open,
	// which means the GPU must be finished executing 
	// the initalization command buffer

	// We will need the command buffer later if we resize
	// the window (to rebuild the depth buffer), so lets
	// not delete the command buffer, but instead clear it,
	// so that can be used again

	// reset the command buffer
	vkResetCommandBuffer(initCmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);

	// same for fence, don't destroy, just reset
	vkResetFences(device, 1, &initFence);
}

void Demo::prepare_render_pass()
{
	// The Render Pass describes what the GPU is outputting.
	// Every time we draw a scene on the graphics card, we want the 
	// graphics card to give us an image (which goes on the screen),
	// and a depth buffer, to make sure objects close to the camera are drawn
	// on top of the objects that are far away from the camera.
	// In previous tutorials, there was only color, but now there is color and depth

	// The initial layout for the color and depth attachments will be LAYOUT_UNDEFINED
	// because at the start of the renderpass, we don't care about their contents.
	// At the start of the subpass, the color attachment's layout will be transitioned
	// to LAYOUT_COLOR_ATTACHMENT_OPTIMAL and the depth stencil attachment's layout
	// will be transitioned to LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL.  At the end of
	// the renderpass, the color attachment's layout will be transitioned to
	// LAYOUT_PRESENT_SRC_KHR to be ready to present.  This is all done as part of
	// the renderpass, no barriers are necessary.
	VkAttachmentDescription attachments[2];

	// The first attachment is our color
	attachments[0].format = format;
	attachments[0].flags = 0;
	attachments[0].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[0].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[0].storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	attachments[0].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[0].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[0].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[0].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	// The second attachment is our depth
	attachments[1].format = depthBufferGPU->format;
	attachments[1].flags = 0;
	attachments[1].samples = VK_SAMPLE_COUNT_1_BIT;
	attachments[1].loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	attachments[1].storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	attachments[1].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	attachments[1].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	attachments[1].finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// For now, the attatchments array is finished, we 
	// will use the array at the bottom of the function, don't
	// worry about it for now

	// Now we need to make what is called a "subpass" which is 
	// part of the render pass. The subpass needs a color reference
	// and a depth reference, so we tell it that the color reference
	// will be the first attachment (0) as we described in the
	// attachments array, and we tell it that depth will be the 
	// second attachment (1), as described by the array above

	VkAttachmentReference color_reference;
	color_reference.attachment = 0;
	color_reference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depth_reference;
	depth_reference.attachment = 1;
	depth_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	// We create the subpass by telling it that we will use this
	// subpass in a GRAPHICS pipeline (and not a compute or raytracing pipeline),
	// we tell it that there will be one color reference, and we give it the 
	// color_reference that we made. In a subpass, there can only be a maximum
	// of one depth references, or there can just be no depth references, so 
	// we do not have to specify that there is only one depth reference.
	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &color_reference;
	subpass.pDepthStencilAttachment = &depth_reference;

	// Now we have our subpass description, 
	// we will use this at the bottom of the function

	// create an array of dependences, this tells the
	// subpass what each attatchment in the subpass depends on
	VkSubpassDependency attachmentDependencies[2];

	// initialize the array as empty
	memset(attachmentDependencies, 0, sizeof(VkSubpassDependency) * 2);

	// The first attachment is our swapchain image, which is what we are
	// outputting to, so that the completed image can get to the screen.
	// We are not moving the image from one stage to another, so the source
	// and the destination are the same, the swapchain image will always
	// be the pipeline's output for color.
	// Image Layout Transition
	attachmentDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	attachmentDependencies[0].dstSubpass = 0;
	attachmentDependencies[0].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	attachmentDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	attachmentDependencies[0].srcAccessMask = 0;
	attachmentDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;

	// The second attachment is the depth buffer
	// Depth buffer is shared between swapchain images
	// We use this depth buffer for tests in the fragment shader, determine the depth
	// of the pixels, to make sure objects close to the camera are drawn over those
	// that are far from the camera. 
	attachmentDependencies[1].srcSubpass = VK_SUBPASS_EXTERNAL;
	attachmentDependencies[1].dstSubpass = 0;
	attachmentDependencies[1].srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	attachmentDependencies[1].dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	attachmentDependencies[1].srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	attachmentDependencies[1].dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;

	// create information that describes what
	// we want in our renderpass
	VkRenderPassCreateInfo rp_info = {};

	// sType for all VkRenderPassCreateInfo structures
	rp_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;

	// the array of pAttatchments will be the "attatchments"
	// array that we just made, and there are 2 elements
	// in the array
	rp_info.attachmentCount = 2;
	rp_info.pAttachments = attachments;

	// The "array" of pSubpasses won't really be an array
	// but we will make it think that it is an array, so we 
	// give it a pointer to the subpass we just made, and we
	// tell it that there is one element
	rp_info.subpassCount = 1;
	rp_info.pSubpasses = &subpass;

	// we give it the attatchment dependencies, there
	// are 2 elements in the attachmentDependencies array
	rp_info.dependencyCount = 2;
	rp_info.pDependencies = attachmentDependencies;

	// create a renderpass based on the information we provided
	vkCreateRenderPass(device, &rp_info, NULL, &render_pass);
}

void Demo::prepare_framebuffers()
{
	// Remember when we had VkImage for the swapchain 
	// images and the depth buffer? Remember how we created
	// a VkImageView to hold each VkImage? Here is where 
	// that is important

	// The framebuffer holds the VkImageViews that
	// will be written to by the GPU at the end of
	// rendering each scene. 
	
	// When we created the swapchain, it was mentioned that
	// the GPU will be swapping which image in the swapchain
	// will be sent to the screen, and which image will be 
	// written to by the GPU.

	// If we put one swapchain image into a framebuffer,
	// we cannot change which swapchain image is in that
	// framebuffer, and the framebuffer tells the GPU 
	// where to render an image to (the swapchain image).

	// Therefore, we need one framebuffer for each swapchain image.

	// We create an array of two attachments,
	// as described in the render pass, one will
	// be used to export color of the image, and
	// the other will be used for writing the depth buffer
	VkImageView attachments[2];

	// The second attachment (at index 1) will be
	// the depth buffer. That will be consistent
	// for every framebuffer
	attachments[1] = depthBufferGPU->imageView;

	// we create a structure of information that will be used
	// to create each framebuffer. sType will be the same
	// for every FrameBufferCreateInfo
	VkFramebufferCreateInfo fb_info = {};
	fb_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;

	// We give it the renderpass, which describes how
	// each attachment will be used (color vs depth)
	fb_info.renderPass = render_pass;

	// We give it the array of attachments,
	// and we mention that there are two elements
	// in the array. Keep in mind that this is a pointer
	// to the array, so we can change the array before
	// submitting the CreateInfo
	fb_info.attachmentCount = 2;
	fb_info.pAttachments = attachments;

	// We give the width and height of the frameBuffer
	// which is the same as the screen dimensions
	fb_info.width = width;
	fb_info.height = height;
	
	// We will only have one layer,
	// don't worry about multpile layers,
	// that is for advacned topics
	fb_info.layers = 1;

	// loop through every swapchain image we have
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		// set the first member of the attachment array (index 0)
		// to the swapchain image that we want to render to, for each
		// framebuffer
		attachments[0] = swapchain_image_resources[i].view;

		// create a framebuffer for each swapchain image
		// based on the information provided.
		// This will be stored in the array of swapchain_image_resources
		// along with the Image, ImageView, and commmandBuffer of each
		// swapchain image. Command buffers for drawing will be explained soon.
		vkCreateFramebuffer(device, &fb_info, NULL, &swapchain_image_resources[i].framebuffer);
	}
}

void Demo::build_swapchain_cmds()
{
	// Create the information needed to start the command buffer,
	// include the VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT flag,
	// which lets us call another command buffer from tihs one. We will
	// call the secondary command buffer from the primary command buffers
	VkCommandBufferBeginInfo cmd_buf_info = {};
	cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

	// Set our clear colors. This sets the background 
	// color to "cornflower blue", which was the default
	// clear color for XNA and MonoGame, it looks nice,
	// but literally this can be anything
	VkClearValue clear_values[2];
	clear_values[0].color.float32[0] = 100.0f / 255.0f;
	clear_values[0].color.float32[1] = 149.0f / 255.0f;
	clear_values[0].color.float32[2] = 237.0f / 255.0f;
	clear_values[0].color.float32[3] = 0.0f;
	
	// reset depth to maximum depth, so that we can draw over it
	clear_values[1].depthStencil.depth = 1.0f;
	clear_values[1].depthStencil.stencil = 0;

	// setup everything we need to begin using a render pass,
	// give it the render pass we made, give it the dimensions
	// of the window, give it the 2 clear values (color and depth)
	VkRenderPassBeginInfo rp_begin = {};
	rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	rp_begin.renderPass = render_pass;
	rp_begin.renderArea.extent.width = width;
	rp_begin.renderArea.extent.height = height;
	rp_begin.clearValueCount = 2;
	rp_begin.pClearValues = clear_values;

	// Get ready to begin a command buffer, the level will
	// be primary, because this is the command buffer that
	// is submitted to the queue, and this is the command buffer
	// that calls the secondary command buffer. We are creating
	// one command buffer at a time, which is why Count is 1,
	// even though we will use this information to make 3 buffers,
	// because they are being made one-at-a-time
	VkCommandBufferAllocateInfo cmdInfo = {};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmdInfo.commandPool = cmd_pool;
	cmdInfo.commandBufferCount = 1;

	// create a new command buffer for each swapchain image.
	// Each command buffer has its own framebuffer, and each framebuffer
	// has its own swapchain image. This allows us to swap command buffers
	// when drawing, which allows us to swap the image we are rendering to,
	// which allows us to utilize the swapchain
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		// create a command buffer
		VkCommandBuffer cmd;

		// we don't need to destroy the secondary cmdBuf, but we need do need to destroy the 
		// swapchain cmdBuf because the framebuffers are being destroyed, we can NOT simply 
		// "reset" the swapchain cmdBuf and give it a new frameBuffer, Exzap from Cemu confirmed this

		// allocate new memory for this command buffer.
		// If this is the first time we are here, then of course it has not been allocated yet.
		// If we are calling prepare() again after resizing the screen, then the command buffer
		// was deleted when we resized the window (because swapchain image was deleted, which
		// deletes the frame buffer and command buffer that correspond) so now we have to 
		// reallocate it
		vkAllocateCommandBuffers(device, &cmdInfo, &cmd);

		// The RenderPassBeginInfo needs a framebuffer to know which
		// image and depth buffer to render to, so give the framebuffer
		// that is in the array of swapchain_image_resources
		rp_begin.framebuffer = swapchain_image_resources[i].framebuffer;

		// begin our command buffer
		// we can now put commands into this command buffer
		vkBeginCommandBuffer(cmd, &cmd_buf_info);

		// the contents are not INLINE,
		// the contents contains SECONDARY_COMMAND_BUFFERS
		// so we need to include the flag that will let us call
		// a secondary command buffer from tihs primary buffer
		vkCmdBeginRenderPass(cmd, &rp_begin, VK_SUBPASS_CONTENTS_SECONDARY_COMMAND_BUFFERS);

		// execute the secondary command buffer
		// from the primary buffer
		vkCmdExecuteCommands(cmd, 1, &drawCmd);

		// Note that ending the renderpass changes the image's layout from
		// COLOR_ATTACHMENT_OPTIMAL to PRESENT_SRC_KHR.
		vkCmdEndRenderPass(cmd);

		// end our command buffer
		vkEndCommandBuffer(cmd);

		// set the swapchain command buffer equal to the
		// cmd that we just created here, and then move on
		// to the next command buffer in the array
		swapchain_image_resources[i].cmd = cmd;
	}
}

Pipe* currentPipe;
VkCommandBuffer* currentCmd;
std::vector<Entity*> entityList;

void Demo::ResetEntityList()
{
	entityList.clear();
}

void Demo::ApplyPipeline2D()
{
	vkCmdBindPipeline(*currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipe2D->pipeline);
	currentPipe = pipe2D;
}

void Demo::ApplyPipelineColor()
{
	vkCmdBindPipeline(*currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeColor->pipeline);
	currentPipe = pipeColor;
}

void Demo::ApplyPipelineBasic()
{
	vkCmdBindPipeline(*currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeBasic->pipeline);
	currentPipe = pipeBasic;
}

void Demo::ApplyPipelineBumpy()
{
	vkCmdBindPipeline(*currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeBumpy->pipeline);
	currentPipe = pipeBumpy;
}

void Demo::ApplyPipelineSky()
{
	vkCmdBindPipeline(*currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeSky->pipeline);
	currentPipe = pipeSky;
}

void Demo::ApplyPipelineWire()
{
	vkCmdBindPipeline(*currentCmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeWire->pipeline);
	currentPipe = pipeWire;
}

void Demo::DrawEntity(Entity* e)
{
	// check to make sure someboby didn't accidentally
	// try to draw the same entity twice. Each entity
	// should only be drawn once. If you are certain 
	// that you wont make a mistake, you can remove this
	for (int i = 0; i < entityList.size(); i++)
		if (entityList[i] == e)
		{
			printf("Error, you tried to draw the same entity twice\n");
			printf("You should make 2 separate Entities with the same Mesh, instead of drawing one entity twice\n");
			system("pause");
			return;
		}

	entityList.push_back(e);
	e->Draw(*currentCmd, currentPipe->pipeline_layout);
}

void Demo::build_secondary_cmd()
{
	// create a command buffer
	// this will be used for our pipeline bindings,
	// draw calls, setting vb / ib, the fun part
	VkCommandBuffer cmd;
	currentCmd = &cmd;

	// build a new command buffer
	// This is a secondary command buffer, because this command
	// buffer isn't given to the queue like the initCmd or the
	// swapchain Cmd, it is executed via the primary command
	// buffer, which is one of the multiple swapchain command buffers
	VkCommandBufferAllocateInfo cmdInfo = {};
	cmdInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	cmdInfo.level = VK_COMMAND_BUFFER_LEVEL_SECONDARY;
	cmdInfo.commandPool = cmd_pool;
	cmdInfo.commandBufferCount = 1;

	// if the program has never been initialized
	// before, then there is no memory allocated for
	// this command buffer
	if (firstInit)
	{
		// use the AllocationInfo above (with the command pool)
		// to allocate new memory for this command buffer
		vkAllocateCommandBuffers(device, &cmdInfo, &cmd);
	}

	// if the program is already initialized, then memory
	// is already allocated for this command buffer.
	// so we don't need to allocate more
	else
	{
		// reset the existing command buffer that is already initialized
		// this clears the commands from the buffer so that we can refill the 
		// buffer with more commands
		vkResetCommandBuffer(drawCmd, VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);
		cmd = drawCmd;
	}

	// this is a secondary command buffer,
	// and it needs to inherit properties from
	// the primary buffer. If you do not include
	// inheritanceInfo, it will still work, but there
	// will be several validation errors. We give it
	// the same renderpass as the primary buffers.
	VkCommandBufferInheritanceInfo info = {};
	info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO;
	info.renderPass = render_pass;

	// To begin this command buffer, we give it the same sType as all
	// command buffers, and we give it the InheritanceInfo that we 
	// just made. We also give it the flags SIMULTANEIOUS_BIT, due 
	// to the fact that this buffer is running at the same time (inside)
	// the primary buffer, and we give it the RENDER_PASS_CONTINUE bit,
	// because we began a render pass in the primary command buffer, and
	// now we are continuing to use that renderpass in the secondary buffer
	VkCommandBufferBeginInfo cmd_buf_info = {};
	cmd_buf_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	cmd_buf_info.pInheritanceInfo = &info;
	cmd_buf_info.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT | VK_COMMAND_BUFFER_USAGE_RENDER_PASS_CONTINUE_BIT;

	// begin the command buffer (drawCmd)
	// based on the information above
	vkBeginCommandBuffer(cmd, &cmd_buf_info);

	// Vulkan renders everything upside-down by default, here we flip
	// the viewport, to render everything right-side-up

	// By default, we would have
	// x = 0
	// y = 0
	// width = width
	// height = height

	// To flip it, we have 
	// x = 0
	// y = height
	// width = width
	// height = -height

	// With this method, we dont need to alter projection matrices
	// like how previous tutorials did. With this setup, it is easy
	// to port existing projects from other APIs to Vulkan

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = (float)height;
	viewport.width = (float)width;
	viewport.height = (float)-height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(cmd, 0, 1, &viewport);

	// Scissor tests clip to a rectangle inside that viewport.
	// If you do not want to clip the image, then leave the 
	// Scissor the way it is. If you want to see what it does, change
	// "width" and "height" to "width/2" and "height/2".
	// That will draw the final image at 100% size, but it will only
	// draw the top-left quadrant of the window. This can be used
	// for black cinematic bars on the screen during cutscenes.

	VkRect2D rect = {};
	rect.offset.x = 0;
	rect.offset.y = 0;
	rect.extent.width = width;
	rect.extent.height = height;
	vkCmdSetScissor(cmd, 0, 1, &rect);

	// add all entities in the scene to the command buffer
	scene->Draw();

	// end command buffer
	// future commands will not go
	// into this command buffer
	vkEndCommandBuffer(cmd);

	// set the secondary-draw
	// command buffer equal to cmd
	drawCmd = cmd;
}

void Demo::prepare()
{
	// We will be calling prepare() multiple times.
	// Some Vulkan assets only need to be created once, like
	// the instance, the device, and the queue (i'll explain those later),
	// while some things need to be destroyed and rebuilt, like the 
	// depth buffer and swapchain images (I'll explain those later).

	// We keep track of a variable called firstInit, to determine
	// if we have run the prepare() function before. "firstInit"
	// is initialized as true at the top of Demo.cpp, and it is
	// set to false after the first initialization is done

	if (firstInit)
	{
		// Validation will tell us if our Vulkan code is correct.
		// Sometimes, our code will execute the way we want it to,
		// but just becasue the code runs, does not mean the code
		// is correct. If you've ever used HTML, you may have used
		// and HTML validator, where your website might look correct,
		// but the validator will tell you that the code is wrong

		// If you run in Debug mode, no Validator text will appear,
		// text from Validator will only appear if you are in Release Mode

		// During development, this is a great tool. However, when it is
		// time to release a software or game, you don't want this running
		// in the background because it will continue constantly checking for errors
		// even if there are no errors. So, when you want to release a software or
		// game, simply set this to false.
		validate = true;

		// During development, it is good to have a console window.
		// You can read errors, and write printf statements.
		// However, if you want to release a software or game, you may
		// not want a console window. Simply comment out this line to 
		// disable the console window.
		prepare_console();

		// set the width and height of the window
		// literally set this to whatever you want
		width = 1280;
		height = 720;

		// give default values to matrices
		projection_matrix = glm::mat4(1);
		view_matrix = glm::mat4(1);

		// build the window with the Win32 API. This will look similar to how
		// a window is created in a DirectX 11/12 engine, and we will use the
		// WndProc from main.cpp to create the window
		prepare_window();

		// We create an instance of Vulkan, this allows us to use VUlkan
		// commands on the CPU, but we will not yet be able to talk to 
		// the graphics device, that comes later
		prepare_instance();

		// Soem Vulkan functions are not available in the SDK's
		// .lib or .dll files, but instead are inside the driver,
		// this is how you get some of them from the instance
		prepare_instance_functionPointers();

		// The physical device gives us all the properties of the GPU
		// that we want to use to render, such as the name of the GPU,
		// how much memory it has, what features it supports, etc.
		// We cannot send commands to the GPU through the PhysicalDevice,
		// but we can use it to determine what our GPU can do.
		prepare_physical_device();

		// we create the surface of Vulkan, which helps Vulkan move a
		// fully-rendered image from the graphics card to the screen
		prepare_surface();

		// The PhysicalDevice and the Device both refer to the same
		// GPU. The difference is that PhysicalDevice tells us the 
		// GPU's properties, while Device allows us to send commands
		// to the GPU.

		// The queue, is exactly what it sounds like. If you have
		// never used a queue before, do a google search on 
		// std::queue to understand the concepts. The queue that
		// we use here is not made with std::queue, but it works
		// the same way. The VkQUeue is the middle-man between
		// C++ and the GPU (VkDevice). C++ will submit commands
		// to the queue, and then the queue will shovel the commands
		// into the GPU. We have to create the Device and the Queue
		// at the same time

		// Create the Device and the Queue
		prepare_device_queue();

		// Soem Vulkan functions are not available in the SDK's
		// .lib or .dll files, but instead are inside the driver,
		// this is how you get some of them from the device
		prepare_device_functionPointers();

		// The Swapchain and currentPresentMode
		// variables will be thoroughly explained
		// in the prepare_swapchain() function

		// sometimes this does not default to NULL
		// so lets set it to NULL here. If this line
		// is removed, then a validation error is risked.
		swapchain = VK_NULL_HANDLE;

		// Set the current present mode of the swapchain
		currentPresentMode = (VkPresentModeKHR)0;
	}

	// build the swapchain, and also
	// prepare the images that are
	// in the swapchain
	prepare_swapchain();

	// If the screen is minimized, do not contineu the function.
	// Exit the prepaer() function, stop drawing to the screen,
	// and come back later when the window is not minimized anymore.
	if (is_minimized)
	{
		prepared = false;
		return;
	}

	// we need to create a commmand pool
	// This command pool will hold all of our commands
	// on the CPU, prior to sending them to the GPU

	// This command pool will be used for all commands
	// in the entire program. At first, commands in this pool
	// will be part of the init-command-buffer, which allows
	// us to send initial buffers (textures and such) from CPU 
	// to GPU

	// Later on, we will use this cmd_pool to handle draw commands,
	// after the inital loading is done

	// create the pool, based on the info provided
	if (firstInit)
	{
		// Get Memory Properteis from our GPU
		// This will tell us how much memory the GPU has,
		// and also tell us information about the memory.
		// Having these properties will enable us to 
		// create and store data in a way that is compatible
		// with the GPU that is currently being used
		vkGetPhysicalDeviceMemoryProperties(gpu, &memory_properties);

		// A command pool is needed to create command buffers,
		// command buffers will handle every command that we want
		// to give to the GPU. Thankfully, creating an empty pool
		// of command buffers only takes 4 lines of code.
		// Later on, we will put command buffers in the command pool

		// VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT
		// allows us to reset command pools, without needing to destroy and reallocate them
		// sType will always be VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO for cmd pools

		// create a structure that holds the information
		// give it the sType, and give it the flag
		VkCommandPoolCreateInfo cmd_pool_info = {};
		cmd_pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		cmd_pool_info.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		// create the command pool, based on the information
		vkCreateCommandPool(device, &cmd_pool_info, NULL, &cmd_pool);

		// create a fence, we don't need to give any 
		// special configuration to this fence, so we
		// just give it the sType
		VkFenceCreateInfo fenceInfo = {};
		fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

		// we create a fence
		// a fence will allow us to control the 
		// flow of the program. The fence will tell
		// us when the GPU is finished executing a job
		vkCreateFence(device, &fenceInfo, NULL, &initFence);
	
		// build the initialization command buffer,
		// which will allow us to send buffers from CPU to GPU,
		// like vertex buffers, index buffers, textures, etc.
		prepare_init_cmd();

		// begin the command buffer,
		// this allows all future commands to be
		// recorded into this buffer. It will be 
		// executed when we are done giving it all the 
		// commands we want (for all buffers, and such).
		begin_init_cmd();

		// A sampler is used by the graphics card to 
		// get each pixel from the texture, depending
		// on what UV coordinates we are looking for.
		prepare_sampler();

		// This is the layout, which will be given to 
		// the pipeline, and it will tell the pipeline to 
		// expect one uniform buffer and one texture
		// for each draw call. If you have 100 different models,
		// if each one uses one uniform buffer and one texture,
		// then there should only be two descriptors here
		prepare_descriptor_layout();

		// this is the descriptor pool, which will tell 
		// the GPU how many different descriptors there will 
		// be throughout the duration of the entire program.
	
		// If you have 100 different models, in the scene
		// if each one uses one uniform buffer and one texture,
		// then there should be 200 descriptors in the pool
		prepare_descriptor_pool();

		// initialize scene (scene.cpp) which loads all
		// of our models and textures
		scene = new Scene();
	}

	// if this is not the first initialization
	// then do not recreate things that we already have,
	// just begin the command buffer again (which we reset
	// the last time we executed it), so now it is empty,
	// and we can use it again. We can use it to build the 
	// depth buffer, which depends on swapchain resolution
	else
	{
		// begin the command buffer,
		// this allows all future commands to be
		// recorded into this buffer. It will be 
		// executed when we are done giving it all the 
		// commands we want (for all buffers, and such).
		begin_init_cmd();
	}

	// This creates the depth buffer.
	// unlike other buffrs that are generated on the CPU
	// and then (sometimes) copied to the GPU, this depth
	// buffer is generated on GPU space, and left alone. 
	// The GPU reads the buffer, the GPU writes to the buffer,
	// so it gets created in GPU memory, then CPU
	// leaves it alone
	prepare_depth_buffer();

	// This end the initialization command buffer,
	// and submit all of the commands that we have
	// been giving to the init_cmd command buffer
	// throughout the prepre() function, including
	// building buffers on the GPU
	execute_init_cmd();

	// After init cmd is executed, many CPU buffers
	// are copied to GPU. We dont need them in RAM
	// anymore because they are copied to VRAM, so
	// we delete them from RAM
	BufferGPU::ClearCacheCPU();
	TextureGPU::ClearCacheCPU();

	// After the game finishes loading for the first
	// time, you can load new meshes, textures, and entities,
	// then call prepare() to send them to GPU. I have 
	// not tested it myself, but memory managment is dynamic
	// enough where it [should] work, unless you run out of
	// room in the descriptor pool, and then you'd just increase
	// the maximum size

	if (firstInit)
	{
		// The renderpass describes what type of
		// data will be outputted by the GPU when
		// it is done rendering a scene. In this example,
		// it will create a color image (which is written
		// to the swapchain), and a depth image, which
		// is written to the depth buffer. We do not provide
		// any buffers for the GPU to write to, we just say
		// what type of data we want to be written
		prepare_render_pass();
	}

	// we prepare the framebuffes, which say 
	// specifically what buffers should be written
	// to by the GPU. This is where we specifically
	// say to write to the depth buffer and
	// swapchain images, by giving the VkImageViews
	// of those images.
	prepare_framebuffers();

	// We only prepare the pipeline once
	// This includes loading shaders
	if (firstInit)
	{
		// We prepare the graphics pipeline, which 
		// describes every stage that the GPU will go
		// through while the scene is being rendered:
		// InputState, Vertex Shader, Fragment Shader,
		// Blending, etc.
		pipeBasic = new PipeBasic(device, desc_layout_basic, render_pass);
		pipeSky = new PipeSky(device, desc_layout_basic, render_pass);
		pipe2D = new Pipe2D(device, desc_layout_basic, render_pass);

		pipeBumpy = new PipeBumpy(device, desc_layout_bumpy, render_pass);
		pipeColor = new PipeColor(device, desc_layout_color, render_pass);
		pipeWire = new PipeWire(device, desc_layout_wire, render_pass);
	}

	// Rather than building 3 command buffers
	// that all have "bind a different swapchain image",
	// "bind the same vertex buffer", "bind the same
	// index buffer", and "make the same draw command",
	// we will have primary and secondary command buffers
	// to optimize this

	// We will have 1 secondary command buffer will hold the 
	// commands to bind pipeline, bind descriptor,
	// bind vertex / index buffers, and draw.
	// There will only be one buffer with these
	// commands, they will be in "drawCmd"
	build_secondary_cmd();

	// After that, we make 3 primary buffers.
	// Each of these command buffers will be
	// "Begin command buffer"
	// "Bind FrameBuffer" (each with a different swapchain image)
	// "Execute the 'drawCmd' secondary command buffer"
	// "End command buffer"

	// We bulid three command buffers, one for each swapchain image.
	// When we build the command buffer, we give it a framebuffer,
	// which has all the image buffers that the GPU will draw to,
	// but after we create a command buffer, we cannot change the framebuffer.
	// So we build one command buffer per swapchain image, then when it
	// is time to execute the command buffers, we swap command buffers,
	// to swap between swapchain images that we are drawing to
	build_swapchain_cmds();

	// We only need to do this the first time
	// the program loads, after that, we can 
	// reset and reuse this again and again
	if (firstInit)
	{
		// This function handles the synchronization of the
		// CPU and GPU, to make sure that one does not get
		// too far ahead of the other.

		// In this function, we create more fences, and 
		// something called "semaphores" to control the flow
		// of the program. To assure the command buffers
		// only draw when they are ready to be drawn,
		// and also let us know when each command buffer 
		// is finished drawing
		prepare_synchronization();
	}

	// Our "repare" functions above may generate pipeline commands
	// that need to be executed before beginning the render loop.
	// Such as copying buffers from CPU to GPU, these commands
	// have been added to the command buffer, and now we execute the buffer

	// set the current buffer (from zero to the number of swapchain images)
	// to zero by default, because that's a good place to start
	current_buffer = 0;

	// our demo is prepared, and ready to start rendering
	prepared = true;

	// Our first initialization is done, so we set this to false.
	// We will call prepare() many times, so we don't want to redo
	// the things that we only need to initailize once. After the
	// first initialization, we only want to redo things that depend
	// on window size (which will be changing):
	// depth buffer, swapchain images, renderpass (which has window dimensions),
	// primary command buffers (which need the new swapchain images), etc.
	firstInit = false;
}

void Demo::delete_resolution_dependencies()
{
	// depth buffer on the GPU
	delete depthBufferGPU;

	// we don't need to destroy the secondary cmdBuf, but we need do need to destroy the 
	// swapchain cmdBuf because the framebuffers are being destroyed, we can NOT simply 
	// "reset" the swapchain cmdBuf and give it a new frameBuffer, Exzap from Cemu confirmed this

	// Loop through each swapchain image
	for (uint32_t i = 0; i < swapchainImageCount; i++)
	{
		// delete the "image" of this swapchain image
		vkDestroyImageView(device, swapchain_image_resources[i].view, NULL);

		// delete the framebuffer that is associated with this swapchain image
		vkDestroyFramebuffer(device, swapchain_image_resources[i].framebuffer, NULL);

		// delee the primary command buffer that is associated with this framebuffer
		vkFreeCommandBuffers(device, cmd_pool, 1, &swapchain_image_resources[i].cmd);
	}

	// delete the array of swapchain_image_resources,
	// which holds all the data that we deleted above ^^
	// so that we can reallocate new images later
	free(swapchain_image_resources);
}

void Demo::resize()
{
	// Do not try to resize the window
	// if we haven't initialized the program yet,
	// that would just be a waste

	// Because WndProc can call resize()
	// prior to initialization (which is rare),
	// we need to check to see if we've already
	// initialized
	if (firstInit == false)
	{
		// If the window is currently minimized, then the 
		// width and height are zero, which means that the
		// resolution-dependent assets were never built,
		// so there would be nothing to delete

		// only delete the resolution-dependent assets
		// if the demo is not currently minimized	

		// Keep in mind, if we are trying to resize the window WHILE is_minimized is false,
		// that means that the program was already minimized, and resize() was called after 
		// trying to un-minimize the window
		if (!is_minimized)
		{
			// we are no longer prepared to render
			prepared = false;

			// vkDeviceWaitIdle actually does not wait until the device is idle, which is stupid.
			// What vkDeviceWaitIdle does, is wait until every queue on the device is idle. The 
			// queue sends command buffers to the GPU with VkSubmitInfo. There is one problem though,
			// the queue can be empty, after the last command buffer is given to the GPU, and then
			// a command buffer will still be in the middle of executing on the GPU while vkDeviceWaitIdle
			// stops waiting (when the queues are empty). 
			vkDeviceWaitIdle(device);

			// To absolutely confirm that all of the GPU's tasks are finished, we need to wait for 
			// the fences to be completed too. 
			
			// However, because the window was minimized (and now it is running this function
			// while we are trying to unminimize the window), we know nothing was being drawn 
			// to the screen while the window was minimized, so we don't need to check the
			// fences to see if a draw is complete, if we know that a draw wasn't started

			// delete only the things that depend on the size
			// of the screen, like depth buffer and swapchain
			delete_resolution_dependencies();
		}
	
		// run the prepare function.
		// firstInit is false, so it will only
		// rebuild things that depend on image size,
		// like depth buffer and swapchain, etc.
		prepare();
	}
}

void Demo::draw()
{
	// Main.cpp passed "keys" to demo, and now
	// this function will pass "keys" to scene
	scene->keys = keys;

	// update scene
	// This updates descriptors, uniform buffers, etc
	// for all entities in the scene
	scene->Update();

	// Update descriptors of all entities that
	// are being drawn, given the data that was 
	// set in Scene->Update. This will make the 
	// data usable by the GPU
	for (int i = 0; i < entityList.size(); i++)
		entityList[i]->Update();

	// When the program is first initialized, we should have an open fence.
	// Aside from that, the fence will only be open if the queue is available.
	// We wait until this fence is open. If it is already open by the time the C++
	// code gets here, then it continues as normal is if the line weren't here.
	// If it is closed, then it means that the Queue is still working on another frame.
	// That means, our C++ code will pause here, and it will not move to the next line,
	// until this fence is open (which will be when the queue is done).

	// Waiting for this fence will confirm that we can draw a frame to this 
	// to this frame index at this time (out of two possible slots).
	vkWaitForFences(device, 1, &drawFences[frame_index], VK_TRUE, UINT64_MAX);

	// If we got past the last line, it means that the fence is open,
	// and we are ready to continue. Picture this in your mind, the 
	// fence is open, so we walk through, and close the fence behind us
	vkResetFences(device, 1, &drawFences[frame_index]);

	// Get the index of the next available swapchain image.
	// When the next image is available, it will trigger the
	// image_aquired_semaphore as complete
	fpAcquireNextImageKHR(device, swapchain, UINT64_MAX,
		image_acquired_semaphores[frame_index], VK_NULL_HANDLE, &current_buffer);

	// Wait for the image acquired semaphore to be signaled to ensure9
	// that the image won't be rendered to until the presentation
	// engine has fully released ownership to the application, and it is
	// okay to render to the image.
	VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

	// We are about to make a submission to the queue.
	// Just like all queues, handles the first submission it gets,
	// then the next, then the next, just like people standing in line at a store.

	// This submission will execute one command buffer, which is 
	// the command buffer that is attached to the swapchain image that we can draw with.
	// This submission will stay in the queue, and wait to be executed, until the 
	// "image_aquired" semaphore is completed by fpAcquireNextImageKHR, because we cannot
	// render to an image until we have one that is available.
	// After the submission is finished executing, it will trigger the draw_complete
	// semaphore as finished

	// Here we cycle between the command buffers for each swapchain image
	// We submit the command buffer that corresponds to the swapchain image
	// that is ready to be drawn to, which we determine with fpAcquireNextImageKHR

	VkSubmitInfo submit_info = {};
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pWaitDstStageMask = &pipe_stage_flags;
	submit_info.waitSemaphoreCount = 1;
	submit_info.pWaitSemaphores = &image_acquired_semaphores[frame_index];
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &swapchain_image_resources[current_buffer].cmd;
	submit_info.signalSemaphoreCount = 1;
	submit_info.pSignalSemaphores = &draw_complete_semaphores[frame_index];

	// Thish fence is currently closed, it will open when
	// the queue's submission is complete
	vkQueueSubmit(queue, 1, &submit_info, drawFences[frame_index]);

	// We are now submitting the command buffer that will draw
	// an image to the screen. Here is how it will work.
	// The command buffer we are submitting will bind a pipeline,
	// the pipeline has a fragment shader that will export color
	// to a framebuffer that was binded to the pipline. This 
	// framebuffer has a swapchain image inside of it, and the
	// swapchain image is connected to the VkSurface, which
	// is connected to our HWND Window, which is on the screen.
	// This repeats every frame, for everything we want to draw

	// We wait for the draw_complete semaphore to finish, because we 
	// shouldn't send an image to the screen if the image is not finished
	// rendering

	VkPresentInfoKHR present = {};
	present.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	present.waitSemaphoreCount = 1;
	present.pWaitSemaphores = &draw_complete_semaphores[frame_index];
	present.swapchainCount = 1;
	present.pSwapchains = &swapchain;
	present.pImageIndices = &current_buffer;

	// submit the presentInfo to the queue.
	// The queue will execute our request to present
	// an image as soon as it is done rendering the
	// image that we want rendered in the command buffer
	fpQueuePresentKHR(queue, &present);

	// increment our frame counter
	frame_index += 1;
	frame_index %= FRAME_LAG;
}

void Demo::run()
{
	// draw the window if our
	// program is prepared to draw

	if (prepared)
		draw();
}


Demo::Demo()
{
	// singleton
	m_pInstance = this;

	// Welcome to the Demo constructor
	// The Demo class will handle the majority
	// of our code in this tutorial

	// Please look at Demo.h, 
	// where you can see a list of every variable that
	// we will use in the Demo class. Every variable
	// in this class will be fully explained while
	// we move through the code

	// The first thing we do is initalize the scene
	prepare();
}

Demo::~Demo()
{
	// vkDeviceWaitIdle actually does not wait until the device is idle, which is stupid.
	// What vkDeviceWaitIdle does, is wait until every queue on the device is idle. The 
	// queue sends command buffers to the GPU with VkSubmitInfo. There is one problem though,
	// the queue can be empty, after the last command buffer is given to the GPU, and then
	// a command buffer will still be in the middle of executing on the GPU while vkDeviceWaitIdle
	// stops waiting (when the queues are empty). 
	vkDeviceWaitIdle(device);

	// To absolutely confirm that all of the GPU's tasks are finished, we need to wait for 
	// the fences to be completed too. 

	// We already called VkWaitForFences with the initFence after the init_cmd buffer was 
	// executed, so we do not need to check for the initFence
	// When we are certain that the GPU is completely idle, we can start deleting things
	vkDestroyFence(device, initFence, NULL);

	for (uint32_t i = 0; i < FRAME_LAG; i++)
	{
		// we wait for draw fences prior to rendering each image,
		// but we do not check fences after, which means that the fences
		// could still be closed, lets wait until they are open, when the 
		// last frame is done rendering

		// wait for the draw fence
		vkWaitForFences(device, 1, &drawFences[i], VK_TRUE, UINT64_MAX);

		// Then we destroy all of the fences that we used for drawing
		vkDestroyFence(device, drawFences[i], NULL);

		// Then we destroy all of the semaphores that were used for drawing
		vkDestroySemaphore(device, image_acquired_semaphores[i], NULL);
		vkDestroySemaphore(device, draw_complete_semaphores[i], NULL);
	}

	// destroy scene, which erases all meshes, entities, textures, etc
	delete scene;

	// Delete the renderpass
	vkDestroyRenderPass(device, render_pass, NULL);

	// destroy pipelines
	delete pipeBumpy;
	delete pipeBasic;
	delete pipeColor;
	delete pipeSky;
	delete pipe2D;
	delete pipeWire;

	// If the window is currently minimized, then the 
	// width and height are zero, which means that the
	// resolution-dependent assets were never built,
	// so there would be nothing to delete

	// only delete the resolution-dependent assets
	// if the demo is not currently minimized
	if (!is_minimized)
	{
		// delete everything that depended on
		// the window's resolution: the depth 
		// buffer, the swapchain images, etc
		delete_resolution_dependencies();
	}

	// destroy the swapchain
	fpDestroySwapchainKHR(device, swapchain, NULL);

	// destroy the sampler
	vkDestroySampler(device, sampler, NULL);

	// destroy the descriptor pool, which holds
	// all of our uniforms. This will also destroy
	// all Descriptor Sets that were in the pool, so we
	// don't need to destroy the descriptor set by ourselves
	vkDestroyDescriptorPool(device, desc_pool, NULL);

	// destroy the layout of the descriptor sets
	vkDestroyDescriptorSetLayout(device, desc_layout_color, NULL);
	vkDestroyDescriptorSetLayout(device, desc_layout_basic, NULL);
	vkDestroyDescriptorSetLayout(device, desc_layout_bumpy, NULL);
	vkDestroyDescriptorSetLayout(device, desc_layout_wire, NULL);

	// Destroy the command pool, because we don't need to 
	// send commands to the GPU anymore. This also destroys
	// all command buffers that used the command pool, so
	// we don't need to free command buffers by ourselves
	vkDestroyCommandPool(device, cmd_pool, NULL);

	// Destroy device, which also destroys queues
	// at the exact same time
	vkDestroyDevice(device, NULL);

	// destroy the surface
	vkDestroySurfaceKHR(inst, surface, NULL);

	// Destroy Vulkan Instance
	vkDestroyInstance(inst, NULL);
}

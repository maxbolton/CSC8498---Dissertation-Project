#pragma once
#include <iostream>
#include "Window.h"
#include "TutorialGame.h"

using namespace NCL;
using namespace CSC8503;

float dt;

int main()
{
	std::cout << "Hello World!" << std::endl;

	
	// Create a window
	WindowInitialisation initInfo;
	initInfo.width = 1280;
	initInfo.height = 720;
	initInfo.windowTitle = "CSC8498 Maxwell Bolton - Procedural Vegetation";

	Window* window = Window::CreateGameWindow(initInfo);

	window->LockMouseToWindow(true);
	window->ShowOSPointer(false);

	if (!window) {
		std::cerr << "Failed to create window!" << std::endl;
		return -1;
	}
	//window->GetTimer().GetTimeDeltaSeconds();

	// Set up the game
	TutorialGame* game = new TutorialGame();
	if (!game) {
		std::cerr << "Failed to create game!" << std::endl;
		return -1;
	}


	while (window->UpdateWindow() && !Window::GetKeyboard()->KeyPressed(KeyCodes::ESCAPE)) {
		//dt = window->GetTimer().GetTimeDeltaSeconds();
		game->UpdateGame(dt);
	}

	Window::DestroyGameWindow();
}
#pragma once

#include "../NCLCoreClasses/KeyboardMouseController.h"
#include "PhysicsSystem.h"
#include "GameTechRenderer.h"
#include "GrassTile.h"



namespace NCL {
	namespace CSC8503 {
		class TutorialGame		{
		public:
			TutorialGame();
			~TutorialGame();

			virtual void UpdateGame(float dt);

		protected:

			void InitialiseAssets();
			void InitCamera();
			void InitWorld();

			void InitDefaultFloor();

			GameObject* AddFloorToWorld(const Vector3& position);
			GameObject* AddSphereToWorld(const Vector3& position, float radius, float inverseMass = 10.0f);
			GameObject* AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass = 10.0f);
			void PlaceGrassBlades(GrassTile* tile);

			GameTechRenderer* renderer;

			PhysicsSystem*		physics;
			GameWorld*			world;

			KeyboardMouseController controller;

			bool useGravity;

			float		forceMagnitude;


			Mesh*	capsuleMesh = nullptr;
			Mesh*	cubeMesh	= nullptr;
			Mesh*	sphereMesh	= nullptr;
			Mesh*	grassBladeMesh = nullptr;

			Texture*	basicTex	= nullptr;
			Shader*		basicShader = nullptr;
			Shader*		grassShader = nullptr;

		};
	}
}


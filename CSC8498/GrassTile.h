#pragma once
#include "GameObject.h"
#include <random>

namespace NCL {
	namespace CSC8503 {

		class GrassTile : public GameObject {
			
		public:
			struct GrassBlade {
				int id;
				Vector3 position;
				Vector3 faceRotation;
				float bendAmount;
			};

		private:
			
			float xLen = 16.0f;
			float yLen = 1.0f;
			float zLen = 16.0f;
			int maxBlades = 10;

			std::vector<GrassBlade> blades;

			

		public:
			GrassTile(Vector3 pos) : posDis(-0.5f, 0.5f),  rotDis(-180.0f, 180.0f), bendDis(-50.0f, 50.0f) {

				AABBVolume* volume = new AABBVolume(Vector3(8, 0, 8));
				this->SetBoundingVolume((CollisionVolume*)volume);
				this->GetTransform()
					.SetScale(Vector3(xLen, yLen, zLen))
					.SetPosition(pos);

				this->SetPhysicsObject(new PhysicsObject(&this->GetTransform(), this->GetBoundingVolume()));
				this->GetPhysicsObject()->SetInverseMass(0);


				CalculateBlades();

			}

			~GrassTile() {
				delete renderObject;
				delete physicsObject;
				delete boundingVolume;
			}

			float* GetXLen() {
				return &xLen;
			}

			float* GetZLen() {
				return &zLen;
			}

			int* GetMaxBlades() {
				return &maxBlades;
			}

			std::vector<GrassBlade>& GetBlades() {
				return blades;
			}

			void CalculateBlades() {
				int id = 0;

				float area = xLen * zLen;
				float density = maxBlades / area;

				int bladesX = static_cast<int>(std::sqrt(density * xLen * zLen));
				int bladesZ = maxBlades / bladesX;



				for (int i = 0; i < bladesX; ++i) {
					for (int j = 0; j < bladesZ; ++j) {
						if (id >= maxBlades) return;
						GrassBlade blade;
						blade.id = id++;
						blade.position = Vector3((i * (xLen / bladesX)) - xLen*0.5, this->GetTransform().GetPosition().y+(yLen*0.5), (j * (zLen / bladesZ)) - zLen * 0.5);
						
						ApplyJitter(blade);
						blades.push_back(blade);
					}
				}
			}


			std::default_random_engine gen;
			std::uniform_real_distribution<float> posDis;
			std::uniform_real_distribution<float> rotDis;
			std::uniform_real_distribution<float> bendDis;

			void ApplyJitter(GrassBlade& blade) {
				gen.seed(std::random_device{}());

				Vector3 posJitter = Vector3(posDis(gen), 0, posDis(gen));
				blade.position += posJitter;

				blade.faceRotation = Vector3(0, rotDis(gen), 0);
				blade.bendAmount = bendDis(gen);
			}
		};

	}

}

#pragma once
#include "GameObject.h"
#include <random>
#include "../FastNoiseLite/Cpp/FastNoiseLite.h"

namespace NCL {
	namespace CSC8503 {

		struct GrassBlade {
			int id;
			Vector3 position;
			Vector3 faceRotation;
			float bendAmount;
			float noiseValue;
		};

		class GrassTile : public GameObject {
			
		public:
			

		private:
			
			float xLen = 16.0f;
			float yLen = 1.0f;
			float zLen = 16.0f;
			int maxBlades = 256;

			std::vector<GrassBlade> blades;

			

		public:
			GrassTile(Vector3 pos) : gen(std::random_device{}()), posDis(-0.5f, 0.5f),  rotDis(-180.0f, 180.0f), bendDis(-2.0f, 2.0f) {


				
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

				FastNoiseLite noise;
				noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
				noise.SetFrequency(0.1f);


				for (int i = 0; i < bladesX; ++i) {
					for (int j = 0; j < bladesZ; ++j) {
						if (id >= maxBlades) return;
						GrassBlade blade;
						blade.id = id++;
						blade.position = Vector3((i * (xLen / bladesX)) - xLen*0.5, this->GetTransform().GetPosition().y+(yLen*0.5), (j * (zLen / bladesZ)) - zLen * 0.5);
						
						ApplyJitter(blade);
						
						blade.noiseValue = noise.GetNoise(blade.position.x, blade.position.z) * 2.0f;

						blades.push_back(blade);
					}
				}
			}


			std::default_random_engine gen;
			std::uniform_real_distribution<float> posDis, rotDis, bendDis;

			void ApplyJitter(GrassBlade& blade) {

				Vector3 posJitter = Vector3(posDis(gen), 0, posDis(gen));
				blade.position += posJitter;

				blade.faceRotation = Vector3(0, rotDis(gen), 0);

				float bend = bendDis(gen);
				//std::cout << "Bend: " << bend << std::endl;
				blade.bendAmount = bend;
			}
		};

	}

}

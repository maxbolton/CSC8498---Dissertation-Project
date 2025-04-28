#pragma once
#include "GameObject.h"
#include <random>
#include "../FastNoiseLite/Cpp/FastNoiseLite.h"
#include "Window.h"
#include "GameTimer.h"
#include "GameTechRenderer.h"
#include "OGLComputeShader.h"

namespace NCL {
	namespace CSC8503 {

		struct GrassBlade {
			int id;
			float padding1;
			Vector3 position;
			float padding2;
			Vector3 faceRotation;
			float bendAmount;
			float noiseValue;
			float padding3;
		};

		class GrassTile : public GameObject {
			
		private:
			
			float xLen = 16.0f;
			float yLen = 1.0f;
			float zLen = 16.0f;
			int maxBlades = 512;

			std::vector<GrassBlade> blades;

			std::default_random_engine gen;
			std::uniform_real_distribution<float> posDis, rotDis, bendDis;

			FastNoiseLite noise;

			bool isCompute;
		//----Compute----//
			OGLComputeShader* computeShader = nullptr;
			GLuint computeShaderProg;
			GLuint grassBladeBuff;


		public:
			GrassTile(Vector3 pos, bool compute = false) : gen(std::random_device{}()), posDis(-0.5f, 0.5f),  rotDis(-180.0f, 180.0f), bendDis(-2.0f, 2.0f) {
				isCompute = compute;


				AABBVolume* volume = new AABBVolume(Vector3(8, 0, 8));
				this->SetBoundingVolume((CollisionVolume*)volume);
				this->GetTransform()
					.SetScale(Vector3(xLen, yLen, zLen))
					.SetPosition(pos);

				this->SetPhysicsObject(new PhysicsObject(&this->GetTransform(), this->GetBoundingVolume()));
				this->GetPhysicsObject()->SetInverseMass(0);


				CalculateBlades();

				if (isCompute) {
					InitCompute();
					DispatchComputeShader(0.0f);
				}
				else {
					noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
					noise.SetFrequency(0.1f);
				}

			}

			~GrassTile() {
				delete renderObject;
				delete physicsObject;
				delete boundingVolume;

				if (isCompute) {
					glDeleteBuffers(1, &grassBladeBuff);
					glDeleteProgram(computeShaderProg);
				}
			}

			void InitCompute() {

				computeShader = new OGLComputeShader("GrassBlade.comp");

				glGenBuffers(1, &grassBladeBuff);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, grassBladeBuff);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GrassBlade) * maxBlades, nullptr, GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, grassBladeBuff);
			}

			float* GetXLen() { return &xLen; }

			float* GetZLen() { return &zLen; }

			int* GetMaxBlades() { return &maxBlades; }

			std::vector<GrassBlade>& GetBlades() { return blades; }

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
						
						if (!isCompute) {
							ApplyJitter(blade);
							blade.noiseValue = noise.GetNoise(blade.position.x, blade.position.z) * 2.0f;
						}
						
						blades.push_back(blade);
					}
				}
			}

			void ApplyJitter(GrassBlade& blade) {

				Vector3 posJitter = Vector3(posDis(gen), 0, posDis(gen));
				blade.position += posJitter;

				blade.faceRotation = Vector3(0, rotDis(gen), 0);

				float bend = bendDis(gen);
				blade.bendAmount = bend;
			}

			void UpdateBlades(float dt) {
				for (GrassBlade& blade : blades) {
					//std::cout << "Timer: " << dt << std::endl;
					blade.noiseValue = noise.GetNoise(blade.position.x + dt * 5.0f, blade.position.z) * 2.0f;
				}
			}

			void DispatchComputeShader(float dt) {
				computeShader->Bind();

				glUniform3f(glGetUniformLocation(computeShader->GetProgramID(), "tilePos"), 
					this->GetTransform().GetPosition().x,
					this->GetTransform().GetPosition().y,
					this->GetTransform().GetPosition().z);
		
				glUniform1f(glGetUniformLocation(computeShader->GetProgramID(), "xLen"), xLen);
				glUniform1f(glGetUniformLocation(computeShader->GetProgramID(), "zLen"), zLen);
				glUniform1i(glGetUniformLocation(computeShader->GetProgramID(), "maxBlades"), maxBlades);
				glUniform1f(glGetUniformLocation(computeShader->GetProgramID(), "time"), dt);

				int workgroupCount = (maxBlades + 255) / 256;
				computeShader->Execute(workgroupCount / 256, 1, 1);

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, grassBladeBuff);
				//glBindBuffer(GL_SHADER_STORAGE_BUFFER, grassBladeBuff);
				//glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(GrassBlade) * maxBlades, blades.data());
			}

		};

	}

}

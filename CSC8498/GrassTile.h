#pragma once
#include "GameObject.h"
#include <random>
#include "../FastNoiseLite/Cpp/FastNoiseLite.h"
#include "Window.h"
#include "GameTimer.h"
#include "GameWorld.h"
#include "MshLoader.h"
#include "RenderObject.h"

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
			
		private:

			bool isCompute = false;
			
			float xLen = 16.0f;
			float yLen = 1.0f;
			float zLen = 16.0f;
			int maxBlades = 512;

			std::vector<GrassBlade> blades;

			FastNoiseLite noise;

			std::default_random_engine gen;
			std::uniform_real_distribution<float> posDis, rotDis, bendDis;

			OGLShader* tileShader;
			OGLMesh* cubeMesh;

			#pragma region Instanced Data
				
			GLuint ssbo;
			OGLShader* bladeShader;
			OGLShader* bladeCompShader;
			OGLMesh* grassBladeMesh;

			OGLTexture* grassTex;
			GLuint* shadowTex;

			GameWorld* gameWorld;
			Window* window;
			//GameTechRenderer* renderer;


			#pragma endregion


		public:
			GrassTile(Vector3 pos, bool compute, GameWorld* gameWorld = nullptr, Window* window = nullptr) : gen(std::random_device{}()), posDis(-0.5f, 0.5f),  rotDis(-180.0f, 180.0f), bendDis(-2.0f, 2.0f) {

				this->isCompute = compute;
				this->gameWorld = gameWorld;
				this->window = window;

				InitAssets();

				SetRenderObject(new RenderObject(&this->GetTransform(), cubeMesh, grassTex, tileShader));
				GetRenderObject()->SetGrassVals(&xLen, &zLen, &maxBlades);

				AABBVolume* volume = new AABBVolume(Vector3(8, 0, 8));
				this->SetBoundingVolume((CollisionVolume*)volume);
				this->GetTransform()
					.SetScale(Vector3(xLen, yLen, zLen))
					.SetPosition(pos);

				this->SetPhysicsObject(new PhysicsObject(&this->GetTransform(), this->GetBoundingVolume()));
				this->GetPhysicsObject()->SetInverseMass(0);

				if (!isCompute) {

					noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
					noise.SetFrequency(0.1f);

					CalculateBlades();
					InstanceGrassBlades();

				}
				else {
					InitSSBO();
				}

				

			}

			~GrassTile() {
				delete renderObject;
				delete physicsObject;
				delete boundingVolume;
			}

			#pragma region Init Methods

			void InitAssets() {
				cubeMesh = LoadMesh("cube.msh");
				tileShader = LoadShader("grassTile.vert", "grassTile.frag");

				bladeShader = LoadShader("grassBlade.vert", "grassBlade.frag");
				bladeCompShader = LoadCompShader("grassBlade.comp");
				grassBladeMesh = LoadMesh("grassBladeCustomSingle (1).msh");
				grassTex = LoadTexture("checkerboard.png");
			}

			OGLShader* LoadShader(const std::string& vertex, const std::string& fragment) {
				return new OGLShader(vertex, fragment);
			}

			OGLShader* LoadCompShader(const std::string& compute) {
				return new OGLShader("", "", compute);
			}

			OGLMesh* LoadMesh(const std::string& name) {
				OGLMesh* mesh = new OGLMesh();
				MshLoader::LoadMesh(name, *mesh);
				mesh->SetPrimitiveType(GeometryPrimitive::Triangles);
				mesh->UploadToGPU();
				return mesh;
			}

			OGLTexture* LoadTexture(const std::string& name) {
				return OGLTexture::TextureFromFile(name).release();
			}

			#pragma endregion

			#pragma region CPU Methods

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
						blade.position = Vector3((i * (xLen / bladesX)) - xLen*0.5, this->GetTransform().GetPosition().y+(yLen*2), (j * (zLen / bladesZ)) - zLen * 0.5);
						
						ApplyJitter(blade);
						
						blade.noiseValue = noise.GetNoise(blade.position.x, blade.position.z) * 2.0f;

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

			void UpdateWind(float dt) {
				for (GrassBlade& blade : blades) {
					//std::cout << "Timer: " << dt << std::endl;
					blade.noiseValue = noise.GetNoise(blade.position.x + dt * 5.0f, blade.position.z) * 2.0f;
				}
			}

			void InstanceGrassBlades() {

				for (GrassBlade& blade : blades) {
					GameObject* bladeObj = new GameObject();
					bladeObj->SetRenderObject(new RenderObject(&bladeObj->GetTransform(), grassBladeMesh, grassTex, bladeShader));
					bladeObj->GetRenderObject()->SetColour(Vector4(Debug::GREEN));
					bladeObj->GetRenderObject()->SetGrassBlade(&blade); // grass blade contains uniform data

					bladeObj->GetTransform().SetPosition(blade.position);


					// apply rotation to blade
					Quaternion rotation = Quaternion::EulerAnglesToQuaternion(blade.faceRotation.x, blade.faceRotation.y, blade.faceRotation.z);
					bladeObj->GetTransform().SetOrientation(rotation);


					bladeObj->GetTransform().SetScale(Vector3(1.0f, 1.0f, 1.0f));
					bladeObj->SetPhysicsObject(new PhysicsObject(&bladeObj->GetTransform(), bladeObj->GetBoundingVolume()));
					bladeObj->GetPhysicsObject()->SetInverseMass(0);

					gameWorld->AddGameObject(bladeObj);
				}
			}
			
			#pragma endregion

			#pragma region GPU Methods
			
			void InitSSBO() {
				/*
					Potential Issues:
					1.	Compute Shader Compilation: Ensure that the compute shader (grassBlade.comp) compiles and links successfully.
					2.	SSBO Binding Point Conflicts: Verify that no other SSBOs are bound to binding point 0 in the same pipeline.
					3.	Workgroup Size: Ensure that the compute shader's declared workgroup size matches the dispatch configuration (256 in this case).
				*/

				// create & bind ssbo
				glGenBuffers(1, &ssbo);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);


				const size_t bytesPerBlade = 16;
				// allocate buffer
				glBufferData(GL_SHADER_STORAGE_BUFFER, bytesPerBlade * maxBlades, nullptr, GL_DYNAMIC_DRAW);
				// make buffer visible to shader (binding point 0)
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);


				// Dispatch Compute

					//calculate number of grass blades along x axis
				int bladesX = static_cast<int>(std::sqrt(maxBlades));
				int bladesZ = maxBlades / bladesX;

				glUseProgram(bladeCompShader->GetProgramID());
				glUniform1ui(glGetUniformLocation(bladeCompShader->GetProgramID(), "bladesX"), bladesX);
				glUniform1ui(glGetUniformLocation(bladeCompShader->GetProgramID(), "bladesZ"), bladesZ);
				glUniform2f(glGetUniformLocation(bladeCompShader->GetProgramID(), "tileSize"), xLen, zLen);

				// Set workgroup size
				GLuint groups = (maxBlades + 255) / 256;
				glDispatchCompute(groups, 1, 1);

				// make sure ssbo writes are visible to draw call
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			}

			void DrawGrass() {
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

				// 1) Bind grass blade vao & ssbo
				glUseProgram(bladeShader->GetProgramID());
				glBindVertexArray(grassBladeMesh->GetVAO());
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

				// 2) use render program (vert & frag)
				glUseProgram(bladeShader->GetProgramID());

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, grassTex->GetObjectID());

				GLint texLoc = glGetUniformLocation(bladeShader->GetProgramID(), "mainTex");
				glUniform1i(texLoc, 0);

				GLint lightDirLoc = glGetUniformLocation(bladeShader->GetProgramID(), "lightDir");
				GLint lightColLoc = glGetUniformLocation(bladeShader->GetProgramID(), "lightColour");

				glUniform3f(lightDirLoc, 0.5f, 1.0f, 0.2f);
				glUniform4f(lightColLoc, 1.0f, 1.0f, 1.0f, 0.0f);

				// 3) upload view-proj mats

				int projLocation = glGetUniformLocation(bladeShader->GetProgramID(), "projMatrix");
				int viewLocation = glGetUniformLocation(bladeShader->GetProgramID(), "viewMatrix");

				Matrix4 viewMatrix = gameWorld->GetMainCamera().BuildViewMatrix();
				Matrix4 projMatrix = gameWorld->GetMainCamera().BuildProjectionMatrix(window->GetScreenAspect());

				glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
				glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

				// 4) Draw n instances
				glDrawElementsInstanced(GL_TRIANGLES, grassBladeMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr, maxBlades);
			}

			void VerifyCompShader() {
				GLuint programID = bladeCompShader->GetProgramID();
				GLint success;

				glGetProgramiv(programID, GL_LINK_STATUS, &success);

				if (!success) {
					char infoLog[512];
					glGetProgramInfoLog(programID, 512, NULL, infoLog);
					std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
				}
				else {
					std::cout << "Shader program linked as expected!" << std::endl;
				}
			}

			void ReadBackSSBO() {
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);
				void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);

				if (ptr) {
					float* data = static_cast<float*>(ptr);
					for (int i = 0; i < maxBlades; ++i) {
						std::cout << "Blade " << i << ": "
							<< "x = " << data[i * 4 + 0] << ", "
							<< "y = " << data[i * 4 + 1] << ", "
							<< "z = " << data[i * 4 + 2] << ", "
							<< "w = " << data[i * 4 + 3] << std::endl;
					}
					glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				}
				else {
					std::cerr << "Failed to map SSBO for reading!" << std::endl;
				}
			}

			#pragma endregion
		};

	}

}

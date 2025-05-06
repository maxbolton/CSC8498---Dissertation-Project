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
			int maxBlades = 1024;

			std::vector<GrassBlade> blades;

			FastNoiseLite noise;

			std::default_random_engine gen;
			std::uniform_real_distribution<float> posDis, rotDis, bendDis;

			OGLShader* tileShader;
			OGLMesh* cubeMesh;

			#pragma region Instanced Data
				
			GLuint ssbo;
			OGLShader* bladeShader;
			OGLShader* instBladeShader;
			OGLShader* bladeCompShader;
			OGLMesh* grassBladeMesh;

			OGLTexture* grassTex;
			GLuint* shadowTex;
			GLuint voronoiTex;

			GameWorld* gameWorld;
			Window* window;


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
					GenVoronoiMap();
					InitSSBO();
				}

				

			}

			~GrassTile() {
				delete renderObject;
				delete physicsObject;
				delete boundingVolume;
			}

			bool GetIsCompute() { return isCompute; }

			#pragma region Init Methods

			void InitAssets() {
				cubeMesh = LoadMesh("cube.msh");
				grassBladeMesh = LoadMesh("grassBladeCustomSingle (1).msh");

				tileShader = LoadShader("grassTile.vert", "grassTile.frag");
				bladeShader = LoadShader("grassBlade.vert", "grassBlade.frag");

				instBladeShader = LoadShader("InstGrassBlade.vert", "InstGrassBlade.frag");
				bladeCompShader = LoadCompShader("grassBlade.comp");

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
						blade.position = Vector3((i * (xLen / bladesX)) - xLen*0.5, this->GetTransform().GetPosition().y+0.5, (j * (zLen / bladesZ)) - zLen * 0.5);
						
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

				glActiveTexture(GL_TEXTURE0+1);
				glBindTexture(GL_TEXTURE_2D, voronoiTex);

				glUniform1i(glGetUniformLocation(bladeCompShader->GetProgramID(), "voronoiMap"), 1);

				// Set workgroup size
				GLuint groups = (maxBlades + 255) / 256;
				glDispatchCompute(groups, 1, 1);

				// make sure ssbo writes are visible to draw call
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				ReadBackSSBO();
			}

			void GenVoronoiMap() {
				noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
				noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add);
				noise.SetSeed(7528);
				noise.SetFrequency(1.0f);

				const int width = 512;
				const int height = 512;

				std::vector<float> voronoiMap(width * height * 4);

				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {

						float u = float(x) / float(width);
						float v = float(y) / float(height);

						float worldX = xLen * u;
						float worldZ = yLen * v;

						float noiseValue = noise.GetNoise(worldX, worldZ);

						float offsetX = (noise.GetNoise(worldX + 0.001f, worldZ) - noiseValue) * 10.0f;
						float offsetZ = (noise.GetNoise(worldX, worldZ + 0.001f) - noiseValue) * 10.0f;

						float distance = fabs(noiseValue);

						float random = rotDis(gen);

						int index = (y * width + x) * 4;
						voronoiMap[index + 0] = offsetX;
						voronoiMap[index + 1] = offsetZ;
						voronoiMap[index + 2] = distance;
						voronoiMap[index + 3] = random;
					}
				}

				glGenTextures(1, &voronoiTex);
				glBindTexture(GL_TEXTURE_2D, voronoiTex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, voronoiMap.data());

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);


			}

			void DrawGrass(GLuint* shadowTex) {
				
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

				// 1) Bind grass blade vao & ssbo
				glUseProgram(instBladeShader->GetProgramID());
				glBindVertexArray(grassBladeMesh->GetVAO());
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);

				// 2) use render program (vert & frag)
				glUseProgram(instBladeShader->GetProgramID());

				// bind main tex
				GLint texLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "mainTex"); // mainTex is valid

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, grassTex->GetObjectID());
				glUniform1i(texLoc, 0);

				// bind shadow tex
				GLint shadowTexLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "shadowTex");
				glActiveTexture(GL_TEXTURE0 + 1);
				glBindTexture(GL_TEXTURE_2D, *shadowTex);
				glUniform1i(shadowTexLoc, 1);


				GLint lightDirLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "lightDir");
				GLint lightColLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "lightColour");

				glUniform3f(lightDirLoc, 0.5f, 1.0f, 0.2f); // pass in light parameter as method args from game tech renderer
				glUniform4f(lightColLoc, 1.0f, 1.0f, 1.0f, 0.0f);

				// 3) upload view-proj mats

				int projLocation = glGetUniformLocation(instBladeShader->GetProgramID(), "projMatrix");
				int viewLocation = glGetUniformLocation(instBladeShader->GetProgramID(), "viewMatrix");

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

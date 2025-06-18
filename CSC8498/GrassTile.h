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

		struct BladeIndex {
			float distance;
			uint32_t index;
		};

		class GrassTile : public GameObject {
			
		private:

			bool isCompute = false;
			
			float xLen = 128.0f;
			float yLen = 1.0f;
			float zLen = 128.0f;
			int maxBlades = 4096*8;

			float windDirX = 1;
			float windDirZ = -0.75;
			float windSpeed = 0.3;

			std::vector<GrassBlade> blades;

			FastNoiseLite noise;

			std::default_random_engine gen;
			std::uniform_real_distribution<float> posDis, rotDis, bendDis;

			OGLShader* tileShader;
			OGLMesh* cubeMesh;

			#pragma region Instanced Data
				
			GLuint ssbo;
			GLuint rotSSBO;
			GLuint uvSSBO;
			GLuint sortedSSBO;

			OGLShader* bladeShader;
			OGLShader* instBladeShader;
			OGLShader* bladeCompShader;
			OGLMesh* grassBladeMesh;
			OGLShader* sortBladeComp;
			OGLShader* initBladeSort;

			OGLTexture* grassTex;

			GLuint voronoiTex;
			OGLTexture* debugVoronoiTex;

			GLuint perlinWindTex;
			OGLTexture* debugPerlinWindTex;

			GLint kLoc;
			GLint jLoc;
			GLint camPosLoc;
			uint32_t groups;

			GameWorld* gameWorld;
			Window* window;


			#pragma endregion


		public:
			GrassTile(Vector3 pos, bool compute, GameWorld* gameWorld = nullptr, Window* window = nullptr) : gen(std::random_device{}()), posDis(-0.5f, 0.5f),  rotDis(-180.0f, 180.0f), bendDis(-2.0f, 2.0f) {

				this->isCompute = compute;
				this->gameWorld = gameWorld;
				this->window = window;

				InitAssets();

				RenderObject* renObj = new RenderObject(&this->GetTransform(), cubeMesh, grassTex, tileShader);
				renObj->SetColour(Vector4(0.35, 0.05, 0.01, 1.0));

				SetRenderObject(renObj);
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
					GenTileableWindNoise();
					debugVoronoiTex = new OGLTexture(voronoiTex);
					InitSSBO();
					InitSortComp();


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
				//grassBladeMesh = LoadMesh("grassBladeCustomSingle (1).msh");
				grassBladeMesh = LoadMesh("grassBladeLeaf.msh");

				tileShader = LoadShader("grassTile.vert", "grassTile.frag");
				bladeShader = LoadShader("grassBlade.vert", "grassBlade.frag");

				instBladeShader = LoadShader("instGrassBlade.vert", "instGrassBlade.frag");
				bladeCompShader = LoadCompShader("grassBlade.comp");
				sortBladeComp = LoadCompShader("bladeSort.comp");
				initBladeSort = LoadCompShader("initBladeSort.comp");

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

			float GetMaxHeight() const {
				float maxHeight = std::numeric_limits<float>::lowest();
				const auto& positions = grassBladeMesh->GetPositionData();
				for (const auto& pos : positions) {
					if (pos.y > maxHeight) {
						maxHeight = pos.y;
					}
				}
				return maxHeight;
			}
			
			#pragma endregion
			#pragma region GPU Methods
			
			void InitSSBO(bool useVoronoi = true, bool useWindNoise = true) {

				// create ssbos
				glGenBuffers(1, &ssbo);
				glGenBuffers(1, &rotSSBO);
				glGenBuffers(1, &uvSSBO);
				glGenBuffers(1, &sortedSSBO);

				// bind pos ssbo
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);

				// allocate buffer
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vector4) * maxBlades, nullptr, GL_DYNAMIC_DRAW);
				// make buffer visible to shader (binding point 0)
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);


				// bind rot ssbo
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, rotSSBO);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vector2) * maxBlades, nullptr, GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, rotSSBO);

				// bind uv ssbo
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, uvSSBO);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Vector2) * maxBlades, nullptr, GL_DYNAMIC_DRAW);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, uvSSBO);

				// bind sorted ssbo
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortedSSBO);
				glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(BladeIndex) * maxBlades, nullptr, GL_DYNAMIC_DRAW);

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


				// set uniform bool
				GLuint vornoiLoc = glGetUniformLocation(bladeCompShader->GetProgramID(), "useVoronoiMap");
				glUniform1i(vornoiLoc, useVoronoi);

				// Set workgroup size
				GLuint groups = (maxBlades + 255) / 256;
				glDispatchCompute(groups, 1, 1);

				// make sure ssbo writes are visible to draw call
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

				#pragma endregion

				//ReadBackSSBO();
			}

			void InitSortComp() {

				// set workgroup size
				groups = ((maxBlades + 255) / 256);

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, sortedSSBO);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo);

				camPosLoc = glGetUniformLocation(initBladeSort->GetProgramID(), "cameraPos");
				kLoc = glGetUniformLocation(sortBladeComp->GetProgramID(), "sortK");
				jLoc = glGetUniformLocation(sortBladeComp->GetProgramID(), "sortJ");

				
				//glMemoryBarrier(GL_BUFFER_UPDATE_BARRIER_BIT);

				//SortBlades();
				//ReadBackSortedSSBO();
			}

			void SortBlades() {


				// Dispatch Compute
				glUseProgram(initBladeSort->GetProgramID());

				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, sortedSSBO);
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo);

				Vector3 cameraPos = gameWorld->GetMainCamera().GetPosition();
				glUniform3fv(camPosLoc, 1, &cameraPos.x);

				glDispatchCompute(groups, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);



				// Sort Blades
				glUseProgram(sortBladeComp->GetProgramID());

				for (uint32_t K = 2; K <= maxBlades; K <<= 1) {

					glUniform1ui(kLoc, K);

					for (uint32_t J = K >> 1; J > 0; J >>= 1) {
						glUniform1ui(jLoc, J);
						glDispatchCompute(groups, 1, 1);

						glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
					}
				}
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

			}

			void GenVoronoiMap() {
				std::random_device rd;
				int seed = rd();

				noise.SetNoiseType(FastNoiseLite::NoiseType_Cellular);
				noise.SetCellularReturnType(FastNoiseLite::CellularReturnType_Distance2Add);
				noise.SetSeed(seed);

				noise.SetFrequency(16.0f);

				const int width = 512;
				const int height = 512;

				std::vector<float> voronoiMap(width * height * 4);

				for (int y = 0; y < height; y++) {
					for (int x = 0; x < width; x++) {

						float u = float(x) / float(width);
						float v = float(y) / float(height);

						float noiseValue = noise.GetNoise(u, v);

						float offsetX = (noise.GetNoise(u + 0.001f, v) - noiseValue);
						float offsetZ = (noise.GetNoise(u, v + 0.001f) - noiseValue);

						float distance = fabs(noiseValue);

						int index = (y * width + x) * 4;
						voronoiMap[index + 0] = offsetX;
						voronoiMap[index + 1] = offsetZ;
						voronoiMap[index + 2] = distance;
						voronoiMap[index + 3] = 0; // leave empty for comp to calc rotation
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

			void DrawGrass(Vector3* lightPos, float* lightRadius, Vector4* lightColour, float dt) {

				GLuint qTotal = 0;
				glGenQueries(1, &qTotal);
				glBeginQuery(GL_SAMPLES_PASSED, qTotal);


				// Bind grass blade vao & ssbo
				glUseProgram(instBladeShader->GetProgramID());
				glBindVertexArray(grassBladeMesh->GetVAO());

				glUniform1i(glGetUniformLocation(instBladeShader->GetProgramID(), "useFront2Back"), true);

				// bind main tex
				GLint texLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "mainTex");

				glActiveTexture(GL_TEXTURE0);
				glBindTexture(GL_TEXTURE_2D, grassTex->GetObjectID());
				glUniform1i(texLoc, 0);

				GLint hasTexLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "hasTexture");
				glUniform1i(hasTexLoc, 0); // enable tex

				GLint objColLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "objectColour");
				glUniform4fv(objColLoc, 1, (const GLfloat*)&Debug::GREEN);

				//// bind shadow tex
				//GLint shadowTexLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "shadowTex");
				//glActiveTexture(GL_TEXTURE0 + 1);
				//glBindTexture(GL_TEXTURE_2D, *shadowTex);
				//glUniform1i(shadowTexLoc, 1);

				GLint maxHeightLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "maxHeight");
				glUniform1f(maxHeightLoc, GetMaxHeight());

				// bind wind noise texture
				glActiveTexture(GL_TEXTURE0 + 2);
				glBindTexture(GL_TEXTURE_2D, perlinWindTex);

				glUniform1i(glGetUniformLocation(instBladeShader->GetProgramID(), "perlinWindTex"), 2);

				GLuint windDir = glGetUniformLocation(instBladeShader->GetProgramID(), "windDir");
				GLuint useWindDirLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "useWindNoise");

				glUniform3f(windDir, windDirX, windDirZ, windSpeed);
				glUniform1i(useWindDirLoc, true);

				GLuint dtLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "deltaTime");
				glUniform1f(dtLoc, dt);


				// bind light properties
				GLint lightPosLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "lightPos");
				GLint lightRadLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "lightRadius");
				GLint lightColLoc = glGetUniformLocation(instBladeShader->GetProgramID(), "lightColour");

				glUniform3fv(lightPosLoc, 1, (float*)lightPos);
				glUniform1f(lightRadLoc, *lightRadius);
				glUniform4fv(lightColLoc, 1, (float*)lightColour);

				// upload view-proj mats
				int projLocation = glGetUniformLocation(instBladeShader->GetProgramID(), "projMatrix");
				int viewLocation = glGetUniformLocation(instBladeShader->GetProgramID(), "viewMatrix");

				Matrix4 viewMatrix = gameWorld->GetMainCamera().BuildViewMatrix();
				Matrix4 projMatrix = gameWorld->GetMainCamera().BuildProjectionMatrix(window->GetScreenAspect());

				glUniformMatrix4fv(projLocation, 1, false, (float*)&projMatrix);
				glUniformMatrix4fv(viewLocation, 1, false, (float*)&viewMatrix);

				// Draw n instances
				glDrawElementsInstanced(GL_TRIANGLES, grassBladeMesh->GetIndexCount(), GL_UNSIGNED_INT, nullptr, maxBlades);

				glEndQuery(GL_SAMPLES_PASSED);


				GLuint samplesThisFrame = 0;
				glGetQueryObjectuiv(qTotal, GL_QUERY_RESULT, &samplesThisFrame);
				std::cout << "Total samples: " << samplesThisFrame << std::endl;
	

			}

			void DrawTile(GLuint* shadowTex, Vector3* lightPos, float* lightRadius, Vector4* lightColour, float dt) {

				SortBlades();

				DrawGrass(lightPos, lightRadius, lightColour, dt);

				//Debug::DrawTex(*debugVoronoiTex, Vector2(12, 12), Vector2(10, 10), Vector4(1.0, 1.0, 1.0, 1.0));
				//Debug::DrawTex(*debugPerlinWindTex, Vector2(12, 35), Vector2(10, 10), Vector4(1.0, 1.0, 1.0, 1.0));
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

			void ReadBackSortedSSBO() {

				glBindBuffer(GL_SHADER_STORAGE_BUFFER, sortedSSBO);
				void* ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
				if (ptr) {
					BladeIndex* data = static_cast<BladeIndex*>(ptr);
					for (int i = 0; i < maxBlades; ++i) {
						std::cout << "Blade " << i << ": "
							<< "distance = " << data[i].distance << ", "
							<< "index = " << data[i].index << std::endl;
					}
					glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
				}
				else {
					std::cerr << "Failed to map SSBO for reading!" << std::endl;
				}
			}

			void GenTileableWindNoise() {
				std::random_device rd;
				int seed = rd();

				noise.SetNoiseType(FastNoiseLite::NoiseType_Perlin);
				noise.SetSeed(seed);
				noise.SetFrequency(0.25f); 

				const int width = 512; 
				const int height = 512;
				const float tileSize = 16.0f;

				std::vector<float> tileableNoise(width * height);

				for (int y = 0; y < height; ++y) {
					for (int x = 0; x < width; ++x) {
						// Map pixel coordinates to noise space
						float u = float(x) / float(width) * tileSize;
						float v = float(y) / float(height) * tileSize;

						// Wrap coordinates to make the noise tileable
						float noiseValue = (noise.GetNoise(u, v) +
							noise.GetNoise(u + tileSize, v) +
							noise.GetNoise(u, v + tileSize) +
							noise.GetNoise(u + tileSize, v + tileSize)) * 0.25f;


						noiseValue += 1.0f; // Normalize to [0, 2]
						noiseValue *= 0.5f; // Scale to [0, 1]


						int index = (y * width + x);
						tileableNoise[index] = noiseValue;
					}
				}

				// Create OpenGL texture
				glGenTextures(1, &perlinWindTex);
				glBindTexture(GL_TEXTURE_2D, perlinWindTex);
				glTexImage2D(GL_TEXTURE_2D, 0, GL_R32F, width, height, 0, GL_RED, GL_FLOAT, tileableNoise.data());

				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
				glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

				// Optionally, store the texture for debugging or further use
				debugPerlinWindTex = new OGLTexture(perlinWindTex);
			}


			#pragma endregion

		};

	}

}

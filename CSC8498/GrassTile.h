#pragma once
#include "GameObject.h"
#include "GameTechRenderer.h"
#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include "Assets.h"



namespace NCL {
	namespace CSC8503 {

		struct GrassBlade {
			Vector4 position;
			Vector4 direction;
		};

		extern float bladeVerts[9];

		class GrassTile : public GameObject {

		private:

			float xLen = 16.0f;
			float yLen = 0.0f;
			float zLen = 16.0f;
			int maxBlades = 256;

			Shader* grassShaderComp = nullptr;


			GLuint grassSSBO;
			GLuint shaderID;
			GLuint computeShader;

			GLuint vertexShader;

		public:
			GrassTile(Vector3 pos) {

				AABBVolume* volume = new AABBVolume(Vector3(8, 0, 8));
				this->SetBoundingVolume((CollisionVolume*)volume);
				this->GetTransform()
					.SetScale(Vector3(16, 1, 16))
					.SetPosition(pos);

				this->SetPhysicsObject(new PhysicsObject(&this->GetTransform(), this->GetBoundingVolume()));
				this->GetPhysicsObject()->SetInverseMass(0);


				InitBladeMesh();
				InitGrassSSBO();

				CompileCompShader();
				LinkCompShader();
				


			}

			~GrassTile() {
				delete renderObject;
				delete physicsObject;
				delete boundingVolume;
			}

			void Update(float dt) {
				glUseProgram(computeShader);
				glUniform1f(glGetUniformLocation(computeShader, "time"), dt);
				glDispatchCompute(maxBlades / 256, 1, 1);
				glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
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

			void InitGrassSSBO() {
				// Create a buffer object
				glGenBuffers(1, &grassSSBO);
				glBindBuffer(GL_SHADER_STORAGE_BUFFER, grassSSBO);
				// Allocate memory for the buffer
				glBufferData(GL_SHADER_STORAGE_BUFFER, maxBlades * sizeof(GrassBlade), NULL, GL_DYNAMIC_DRAW);
				// Bind the buffer to the shader storage block
				glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, grassSSBO);
			}

			void InitBladeMesh() {
				GLuint bladeVBO, bladeVAO;
				glGenVertexArrays(1, &bladeVAO);
				glGenBuffers(1, &bladeVBO);

				glBindVertexArray(bladeVAO);
				glBindBuffer(GL_ARRAY_BUFFER, bladeVBO);
				glBufferData(GL_ARRAY_BUFFER, sizeof(bladeVerts), bladeVerts, GL_STATIC_DRAW);

				// location 0 = baseVertex
				glEnableVertexAttribArray(0);
				glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);
			}

			void CompileCompShader() {
				shaderID = glCreateShader(GL_COMPUTE_SHADER);
				std::string computeSource = LoadFileAsString("GrassBlade.comp");
				const char* source = computeSource.c_str();
				glShaderSource(shaderID, 1, &source, nullptr);
				glCompileShader(shaderID);

				GLint compileStatus;
				glGetShaderiv(shaderID, GL_COMPILE_STATUS, &compileStatus);

				if (!compileStatus) {
					GLint logLength;
					glGetShaderiv(shaderID, GL_INFO_LOG_LENGTH, &logLength);
					std::string log(logLength, ' ');
					glGetShaderInfoLog(shaderID, logLength, nullptr, log.data());
					std::cerr << "Compute shader compilation failed: " << log << std::endl;
				}

				std::cout << "Compute shader compiled successfully!" << std::endl;

			}

			void LinkCompShader() {
				computeShader = glCreateProgram();
				glAttachShader(computeShader, shaderID);
				glLinkProgram(computeShader);

				GLint linkStatus;
				glGetProgramiv(computeShader, GL_LINK_STATUS, &linkStatus);
				if (!linkStatus) {
					GLint logLength;
					glGetProgramiv(computeShader, GL_INFO_LOG_LENGTH, &logLength);
					std::string log(logLength, ' ');
					glGetProgramInfoLog(computeShader, logLength, nullptr, log.data());
					std::cerr << "Compute shader linking failed: " << log << std::endl;
				}
			}

			void CompileVertexShader() {

			}


			std::string LoadFileAsString(const std::string& filePath) {
				std::ifstream fileStream(Assets::SHADERDIR+filePath);
				if (!fileStream.is_open()) {
					throw std::runtime_error("Could not open file: " + filePath);
				}

				std::stringstream buffer;
				buffer << fileStream.rdbuf();
				return buffer.str();
			}

		};

	}
	

}

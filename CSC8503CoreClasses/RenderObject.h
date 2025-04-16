#pragma once
#include "Texture.h"
#include "Shader.h"
#include "Mesh.h"

namespace NCL {
	using namespace NCL::Rendering;

	namespace CSC8503 {
		class Transform;
		using namespace Maths;

		class RenderObject
		{
		public:
			RenderObject(Transform* parentTransform, Mesh* mesh, Texture* tex, Shader* shader);
			~RenderObject();

			void SetDefaultTexture(Texture* t) {
				texture = t;
			}

			Texture* GetDefaultTexture() const {
				return texture;
			}

			Mesh*	GetMesh() const {
				return mesh;
			}

			Transform*		GetTransform() const {
				return transform;
			}

			Shader*		GetShader() const {
				return shader;
			}

			void SetColour(const Vector4& c) {
				colour = c;
			}

			Vector4 GetColour() const {
				return colour;
			}

			float* GetXLen() const {
				return xLen;
			}

			float* GetZLen() const {
				return zLen;
			}

			int* GetMaxBlades() const {
				return maxBlades;
			}

			float* GetBendAmount() const {
				return bendAmount;
			}

			void SetGrassVals(float* xLen, float* zLen, int* maxBlades) {
				this->xLen = xLen;
				this->zLen = zLen;
				this->maxBlades = maxBlades;
			}

			float GetMaxHeight() const {
				float maxHeight = std::numeric_limits<float>::lowest();
				const auto& positions = mesh->GetPositionData();
				for (const auto& pos : positions) {
					if (pos.y > maxHeight) {
						maxHeight = pos.y;
					}
				}
				return maxHeight;
			}

			void SetBendAmount(float* bendAmount) {
				this->bendAmount = bendAmount;
			}

		protected:
			Mesh*		mesh;
			Texture*	texture;
			Shader*		shader;
			Transform*	transform;
			Vector4		colour;

			float* xLen = nullptr;
			float* zLen = nullptr;
			int* maxBlades = nullptr;
			float* bendAmount = nullptr;


		};
	}
}

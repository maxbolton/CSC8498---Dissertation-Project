#pragma once
#include "GameObject.h"

namespace NCL {
	namespace CSC8503 {

		class GrassTile : public GameObject {
			
		private:
			
			const float xLen = 16.0f;
			const float yLen = 0.0f;
			const float zLen = 16.0f;



		public:
			GrassTile(Vector3 pos) {

				AABBVolume* volume = new AABBVolume(Vector3(8, 0, 8));
				this->SetBoundingVolume((CollisionVolume*)volume);
				this->GetTransform()
					.SetScale(Vector3(16, 1, 16))
					.SetPosition(pos);

				this->SetPhysicsObject(new PhysicsObject(&this->GetTransform(), this->GetBoundingVolume()));
				this->GetPhysicsObject()->SetInverseMass(0);

			}

			~GrassTile() {
				delete renderObject;
				delete physicsObject;
				delete boundingVolume;
			}




		};

	}

}

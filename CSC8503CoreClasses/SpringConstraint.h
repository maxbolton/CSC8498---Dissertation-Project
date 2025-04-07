#pragma once
#include "Constraint.h"
#include "Camera.h"

namespace NCL {
	namespace CSC8503 {
		class GameObject;

		class SpringConstraint : public Constraint {
		public:
			SpringConstraint(GameObject* player, Camera* cam, float d);
			~SpringConstraint();

			void UpdateConstraint(float dt) override;

		protected:
			GameObject* player;
			Camera* camera;

			float targetDistance;
		};
	}
}
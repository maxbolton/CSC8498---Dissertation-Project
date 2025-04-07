#include "SpringConstraint.h"

#include "GameObject.h"
#include "Camera.h"
#include "PhysicsObject.h"

using namespace NCL;
using namespace Maths;
using namespace CSC8503;

SpringConstraint::SpringConstraint(GameObject* player, Camera* cam, float d) {
	this->player = player;
	this->camera = cam;
	targetDistance = d;
}

SpringConstraint::~SpringConstraint() {
}

void SpringConstraint::UpdateConstraint(float dt) {
	Vector3 playerPos = player->GetTransform().GetPosition();
	Vector3 cameraPos = camera->GetTransform().GetPosition();

	cameraPos.y = playerPos.y + 30.0f;
	// apply offset to z-axis to keep player in view of camera
	Vector3 relativePos = Vector3(playerPos.x, playerPos.y, playerPos.z+50) - cameraPos;
	relativePos.y = 0.0f;

	float currentDistance = Vector::Length(relativePos);

	float offset = targetDistance - currentDistance;

	if (abs(offset) > 0.0f) {
		Vector3 offsetDir = Vector::Normalise(relativePos);

		PhysicsObject* playerPhys = player->GetPhysicsObject();
		PhysicsObject* camPhys = camera->GetPhysicsObject();

		Vector3 relativeVelocity = playerPhys->GetLinearVelocity() - camPhys->GetLinearVelocity();
		relativeVelocity.y = 0.0f;

		float constraintMass = playerPhys->GetInverseMass() + camPhys->GetInverseMass();

		if (constraintMass > 0.0f) {
			// How much of their relative force is affecting the constraint
			float velocityDot = Vector::Dot(relativeVelocity, offsetDir);

			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;

			float lambda = -(velocityDot + bias) / constraintMass;

			Vector3 camImpulse = -offsetDir * lambda;

			//apply damping to avoid oscillation
			camImpulse -= camPhys->GetLinearVelocity() * 0.1f;

			camPhys->ApplyLinearImpulse(camImpulse); // multiplied by mass

		}
	}

	camera->GetTransform().SetPosition(cameraPos);
}
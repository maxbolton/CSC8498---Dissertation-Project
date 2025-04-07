#include "PositionConstraint.h"
//#include "../../Common/Vector3.h"
#include "GameObject.h"
#include "PhysicsObject.h"
//#include "Debug.h"



using namespace NCL;
using namespace Maths;
using namespace CSC8503;

PositionConstraint::PositionConstraint(GameObject* a, GameObject* b, float d)
{
	objectA		= a;
	objectB		= b;
	distance	= d;
}

PositionConstraint::~PositionConstraint()
{

}

//a simple constraint that stops objects from being more than <distance> away
//from each other...this would be all we need to simulate a rope, or a ragdoll
void PositionConstraint::UpdateConstraint(float dt)	{
	Vector3 relativePos = objectA->GetTransform().GetPosition() - objectB->GetTransform().GetPosition();
	
	float currentDistance = Vector::Length(relativePos);

	float offset = distance - currentDistance;

	if (abs(offset) > 0.0f) {
		Vector3 offsetDir = Vector::Normalise(relativePos);

		PhysicsObject* physA = objectA->GetPhysicsObject();
		PhysicsObject* physB = objectB->GetPhysicsObject();

		Vector3 relativeVelcotiy = physA->GetLinearVelocity() - physB->GetLinearVelocity();

		float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();

		if (constraintMass > 0.0f) {
			// How much of their relative force is affecting the constraint
			float velocityDot = Vector::Dot(relativeVelcotiy, offsetDir);

			float biasFactor = 0.01f;
			float bias = -(biasFactor / dt) * offset;

			float lambda = -(velocityDot + bias) / constraintMass;

			Vector3 aImpulse = offsetDir * lambda;
			Vector3 bImpulse = -offsetDir * lambda;

			physA->ApplyLinearImpulse(aImpulse); // multiplied by mass
			physB->ApplyLinearImpulse(bImpulse); // multiplied by mass
		}
	}
}


FollowStringConstraint::FollowStringConstraint(GameObject* a, GameObject* b, float d)
{
	objectA = a;
	objectB = b;
	distance = d;
}

FollowStringConstraint::~FollowStringConstraint()
{

}

void FollowStringConstraint::UpdateConstraint(float dt) {
    // Calculate forward vector
    Vector3 forward = objectA->GetTransform().GetOrientation() * Vector3(0, 0, 1);

    // Desired position for objectB
    Vector3 objectAPos = objectA->GetTransform().GetPosition();
    Vector3 targetPos = objectAPos - forward * distance;

    // Calculate distance and offset
    Vector3 relativePos = objectB->GetTransform().GetPosition() - targetPos;
    float currentDistance = Vector::Length(relativePos);
    float offset = currentDistance - distance;

    if (abs(offset) > 0.01f) { // Threshold to avoid jitter
        Vector3 offsetDir = Vector::Normalise(relativePos);

        PhysicsObject* physA = objectA->GetPhysicsObject();
        PhysicsObject* physB = objectB->GetPhysicsObject();

        Vector3 relativeVelocity = physA->GetLinearVelocity() - physB->GetLinearVelocity();

        float constraintMass = physA->GetInverseMass() + physB->GetInverseMass();

        if (constraintMass > 0.0f) {
            float velocityDot = Vector::Dot(relativeVelocity, offsetDir);

            float biasFactor = 0.01f;
            float bias = -(biasFactor / dt) * offset;

            float lambda = -(velocityDot + bias) / constraintMass;

            Vector3 aImpulse = offsetDir * lambda;
            Vector3 bImpulse = -offsetDir * lambda;

            // Damping
            float dampingFactor = 0.1f;
            aImpulse -= physA->GetLinearVelocity() * dampingFactor;
            bImpulse -= physB->GetLinearVelocity() * dampingFactor;

            // Clamp impulses
            float maxImpulse = 10.0f;
            if (Vector::Length(aImpulse) > maxImpulse) {
                aImpulse = Vector::Normalise(aImpulse) * maxImpulse;
            }
            if (Vector::Length(bImpulse) > maxImpulse) {
                bImpulse = Vector::Normalise(bImpulse) * maxImpulse;
            }

            // Apply impulse
            physB->ApplyLinearImpulse(bImpulse);
        }
    }

    //rotate enemy to face next point
    Quaternion currentOrientation = objectB->GetTransform().GetOrientation();
    Quaternion targetOrientation = Quaternion::EulerAnglesToQuaternion(0, atan2(forward.x, forward.z) * 180.0f / 3.14159265358979323846f, 0);

    // Ensure the shortest path is taken
    if (Quaternion::Dot(currentOrientation, targetOrientation) < 0.0f) {
        targetOrientation = -targetOrientation;
    }
    objectB->GetTransform().SetOrientation(Quaternion::Slerp(currentOrientation, targetOrientation, 0.25f));


}

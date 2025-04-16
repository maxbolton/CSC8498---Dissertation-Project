#include "TutorialGame.h"
#include "GameWorld.h"
#include "PhysicsObject.h"
#include "RenderObject.h"
#include "TextureLoader.h"
#include "GrassTile.h"



using namespace NCL;
using namespace CSC8503;

TutorialGame::TutorialGame() : controller(*Window::GetWindow()->GetKeyboard(), *Window::GetWindow()->GetMouse()) {
	world		= new GameWorld();
	renderer = new GameTechRenderer(*world);

	physics		= new PhysicsSystem(*world);

	forceMagnitude	= 10.0f;
	useGravity		= false;

	world->GetMainCamera().SetController(controller);

	controller.MapAxis(0, "Sidestep");
	controller.MapAxis(1, "UpDown");
	controller.MapAxis(2, "Forward");

	controller.MapAxis(3, "XLook");
	controller.MapAxis(4, "YLook");

	InitialiseAssets();
}


void TutorialGame::InitialiseAssets() {
	cubeMesh	= renderer->LoadMesh("cube.msh");
	sphereMesh	= renderer->LoadMesh("sphere.msh");
	capsuleMesh = renderer->LoadMesh("capsule.msh");
	grassBladeMesh = renderer->LoadMesh("grassBladeCustom.msh");

	basicTex	= renderer->LoadTexture("checkerboard.png");
	basicShader = renderer->LoadShader("scene.vert", "scene.frag");
	grassShader = renderer->LoadShader("grassTile.vert", "grassTile.frag");

	InitCamera();
	InitWorld();
}

TutorialGame::~TutorialGame()	{
	delete cubeMesh;
	delete sphereMesh;

	delete basicTex;
	delete basicShader;

	delete physics;
	delete renderer;
	delete world;
}

void TutorialGame::UpdateGame(float dt) {

	if (Window::GetKeyboard()->KeyPressed(KeyCodes::R)) {
		InitWorld();
		std::cout << "Resetting World " << std::endl;
	}

	world->GetMainCamera().UpdateCamera(dt);

	//This year we can draw debug textures as well!
	//Debug::DrawTex(*basicTex, Vector2(10, 10), Vector2(5, 5), Debug::MAGENTA);

	world->UpdateWorld(dt);
	renderer->Update(dt);
	physics->Update(dt);

	renderer->Render();
	Debug::UpdateRenderables(dt);
}

void TutorialGame::InitCamera() {
	world->GetMainCamera().SetNearPlane(0.1f);
	world->GetMainCamera().SetFarPlane(500.0f);
	world->GetMainCamera().SetPitch(-15.0f);
	world->GetMainCamera().SetYaw(315.0f);
	world->GetMainCamera().SetPosition(Vector3(-60, 40, 60));
}

void TutorialGame::InitWorld() {
	world->ClearAndErase();
	physics->Clear();

	//InitMixedGridWorld(15, 15, 3.5f, 3.5f);
	//InitGameExamples();
	InitDefaultFloor();

	GrassTile* grassTile = new GrassTile(Vector3(0, 2, 0));

	grassTile->SetRenderObject(new RenderObject(&grassTile->GetTransform(), cubeMesh, basicTex, grassShader));
	grassTile->GetRenderObject()->SetGrassVals(grassTile->GetXLen(), grassTile->GetZLen(), grassTile->GetMaxBlades());
	PlaceGrassBlades(grassTile);

	world->AddGameObject(grassTile);

}

void TutorialGame::PlaceGrassBlades(GrassTile* tile) {

	for (GrassTile::GrassBlade blade : tile->GetBlades()) {
		GameObject* bladeObj = new GameObject();
		bladeObj->SetRenderObject(new RenderObject(&bladeObj->GetTransform(), grassBladeMesh, basicTex, basicShader));
		bladeObj->GetRenderObject()->SetColour(Vector4(Debug::GREEN));
		bladeObj->GetRenderObject()->SetGrassVals(tile->GetXLen(), tile->GetZLen(), tile->GetMaxBlades());
		bladeObj->GetTransform().SetPosition(blade.position);

		// apply rotation to blade
		Quaternion rotation = Quaternion::EulerAnglesToQuaternion(blade.rotation.x, blade.rotation.y, blade.rotation.z);
		bladeObj->GetTransform().SetOrientation(rotation);


		bladeObj->GetTransform().SetScale(Vector3(0.5f, 0.5f, 0.5f));
		bladeObj->SetPhysicsObject(new PhysicsObject(&bladeObj->GetTransform(), bladeObj->GetBoundingVolume()));
		bladeObj->GetPhysicsObject()->SetInverseMass(0);
		world->AddGameObject(bladeObj);
	}
}

/*

A single function to add a large immoveable cube to the bottom of our world

*/
GameObject* TutorialGame::AddFloorToWorld(const Vector3& position) {
	GameObject* floor = new GameObject();

	floor->SetCollisionLayer(CollisionLayer::Terrain);

	Vector3 floorSize = Vector3(200, 2, 200);
	AABBVolume* volume = new AABBVolume(floorSize);
	floor->SetBoundingVolume((CollisionVolume*)volume);
	floor->GetTransform()
		.SetScale(floorSize * 2.0f)
		.SetPosition(position);

	floor->SetRenderObject(new RenderObject(&floor->GetTransform(), cubeMesh, basicTex, basicShader));
	floor->SetPhysicsObject(new PhysicsObject(&floor->GetTransform(), floor->GetBoundingVolume()));

	floor->GetPhysicsObject()->SetInverseMass(0);
	floor->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(floor);

	return floor;
}

GameObject* TutorialGame::AddSphereToWorld(const Vector3& position, float radius, float inverseMass) {
	GameObject* sphere = new GameObject();

	Vector3 sphereSize = Vector3(radius, radius, radius);
	SphereVolume* volume = new SphereVolume(radius);
	sphere->SetBoundingVolume((CollisionVolume*)volume);

	sphere->GetTransform()
		.SetScale(sphereSize)
		.SetPosition(position);

	sphere->SetRenderObject(new RenderObject(&sphere->GetTransform(), sphereMesh, basicTex, basicShader));
	sphere->SetPhysicsObject(new PhysicsObject(&sphere->GetTransform(), sphere->GetBoundingVolume()));

	sphere->GetPhysicsObject()->SetInverseMass(inverseMass);
	sphere->GetPhysicsObject()->InitSphereInertia();

	world->AddGameObject(sphere);

	return sphere;
}

GameObject* TutorialGame::AddCubeToWorld(const Vector3& position, Vector3 dimensions, float inverseMass) {
	GameObject* cube = new GameObject();

	AABBVolume* volume = new AABBVolume(dimensions);
	cube->SetBoundingVolume((CollisionVolume*)volume);

	cube->GetTransform()
		.SetPosition(position)
		.SetScale(dimensions * 2.0f);

	cube->SetRenderObject(new RenderObject(&cube->GetTransform(), cubeMesh, basicTex, basicShader));
	cube->SetPhysicsObject(new PhysicsObject(&cube->GetTransform(), cube->GetBoundingVolume()));

	cube->GetPhysicsObject()->SetInverseMass(inverseMass);
	cube->GetPhysicsObject()->InitCubeInertia();

	world->AddGameObject(cube);

	return cube;
}

void TutorialGame::InitDefaultFloor() {
	AddFloorToWorld(Vector3(0, 0, 0));
}


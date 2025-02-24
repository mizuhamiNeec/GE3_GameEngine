#pragma once
#include <memory>

#include <Components/PlayerMovement.h>
#include <Components/MeshRenderer/StaticMeshRenderer.h>

#include <Entity/Base/Entity.h>

#include <Object3D/Object3D.h>
#include <Object3D/Object3DCommon.h>

#include <Particle/ParticleObject.h>

#include <Physics/Physics.h>

#include <Renderer/Renderer.h>

#include <Scene/Base/Scene.h>

#include <Sprite/Sprite.h>
#include <Sprite/SpriteCommon.h>

#include "Components/ColliderComponent/BoxColliderComponent.h"
#include "Components/ColliderComponent/MeshColliderComponent.h"

#include "Physics/PhysicsEngine.h"

class EnemyMovement;
class CameraRotator;
class CameraSystem;

class GameScene : public Scene {
public:
	~GameScene() override = default;
	void Init() override;
	void Update(float deltaTime) override;
	void Render() override;
	void Shutdown() override;

private:
	D3D12* renderer_ = nullptr;

	std::unique_ptr<Sprite> sprite_;

	std::unique_ptr<Entity> cameraRoot_;
	CameraRotator* cameraRotator_ = nullptr;

	std::unique_ptr<Entity> camera_;
	std::shared_ptr<CameraComponent> cameraComponent_;

	std::unique_ptr<Entity> entPlayer_;
	std::shared_ptr<PlayerMovement> playerMovement_;
	std::shared_ptr<BoxColliderComponent> playerCollider_;

	std::unique_ptr<Entity> testMeshEntity_;
	std::shared_ptr<StaticMeshRenderer> floatTestMR_;
	StaticMesh* debugMesh = nullptr;

	std::unique_ptr<Entity> debugTestMeshEntity_;
	std::shared_ptr<StaticMeshRenderer> debugTestMR_;
	std::shared_ptr<MeshColliderComponent> debugTestMeshCollider_;

	std::vector<Triangle> worldMesh_;

	std::unique_ptr<PhysicsEngine> physicsEngine_;
};

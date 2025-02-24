#pragma once
#include <Components/Base/Component.h>
#include <Components/Transform/TransformComponent.h>

#include <Lib/Math/MathLib.h>
#include <Lib/Math/Matrix/Mat4.h>

class CameraComponent : public Component {
public:
	~CameraComponent() override;

	void OnAttach(Entity& owner) override;
	void Update(float deltaTime) override;
	void DrawInspectorImGui() override;

	float& GetFovVertical();
	void SetFovVertical(const float& newFovVertical);

	float& GetZNear();
	void SetNearZ(const float& newNearZ);

	float& GetZFar();
	void SetFarZ(const float& newFarZ);

	Mat4& GetViewProjMat();
	Mat4& GetViewMat();
	Mat4& GetProjMat();

	float& GetAspectRatio();
	void SetAspectRatio(float newAspectRatio);

private:
	TransformComponent* transform_ = nullptr;

	float fov_ = 90.0f * Math::deg2Rad;
	float aspectRatio_ = 0.0f;
	float zNear_ = 0.25f;
	float zFar_ = 65536.0f;

	Mat4 worldMat_;
	Mat4 viewMat_;
	Mat4 projMat_;
	Mat4 viewProjMat_;
};


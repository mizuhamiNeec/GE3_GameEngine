#pragma once
#include "Math/MathLib.h"
#include "Structs/Structs.h"

class Window;

class Camera {
public:
	Camera();

	void Update();

	// Setter
	void SetPos(const Vec3& newPos);
	void SetRotate(const Vec3& newRot);
	void SetFovVertical(const float& newFovVertical);
	void SetFovHorizontal(const float& newFovHorizontal);
	void SetNearZ(const float& newNearZ);
	void SetFarZ(const float& newFarZ);
	void SetAspectRatio(float newAspectRatio);

	// Getter
	Vec3& GetPos();
	Vec3& GetRotate();
	float& GetFovHorizontal();
	float& GetFovVertical();
	float& GetNearZ();
	float& GetFarZ();
	Mat4& GetViewProjMat();

private:
	float hFov_ = 0.0f;
	float vFov_ = 90.0f * Math::deg2Rad;
	float aspectRatio_ = 0.0f;
	float nearZ_ = 0.1f;
	float farZ_ = 100.0f;

	Transform transform_{
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	Mat4 worldMat_;
	Mat4 viewMat_;
	Mat4 projMat_;
	Mat4 viewProjMat_;

	Window* window_ = nullptr;
};
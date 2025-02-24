#pragma once
#include "../Lib/Math/MathLib.h"
#include "../Lib/Structs/Structs.h"

class Window;

class Camera {
public:
	Camera();

	void Update();

	// Setter
	void SetTransform(const Transform& newTransform);
	void SetPos(const Vec3& newPos);
	void SetRotate(const Vec3& newRot);
	void SetFovVertical(const float& newFovVertical);
	void SetNearZ(const float& newNearZ);
	void SetFarZ(const float& newFarZ);
	void SetAspectRatio(float newAspectRatio);

	// Getter
	Transform& GetTransform();
	Vec3& GetPos();
	Vec3& GetRotate();
	float& GetFovVertical();
	float& GetZNear();
	float& GetZFar();
	Mat4& GetViewProjMat();
	Mat4& GetViewMat();
	Mat4& GetProjMat();

private:
	float fov_ = 90.0f * Math::deg2Rad;
	float aspectRatio_ = 0.0f;
	float zNear_ = 0.1f;
	float zFar_ = 1024.0f;

	Transform transform_{
		{1.0f, 1.0f, 1.0f},
		{0.0f, 0.0f, 0.0f},
		{0.0f, 0.0f, 0.0f}
	};

	Mat4 worldMat_;
	Mat4 viewMat_;
	Mat4 projMat_;
	Mat4 viewProjMat_;
};

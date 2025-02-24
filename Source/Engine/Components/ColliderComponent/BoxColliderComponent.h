#pragma once

#include <Components/ColliderComponent/Base/ColliderComponent.h>

class BoxColliderComponent : public ColliderComponent {
public:
	BoxColliderComponent();
	~BoxColliderComponent() override = default;

	void OnAttach(Entity& owner) override;
	void Update(float deltaTime) override;
	void DrawInspectorImGui() override;

	bool CheckCollision(const ColliderComponent* other) const override;
	bool IsDynamic() override;

	// コライダーのサイズを設定
	void SetSize(const Vec3& size);
	[[nodiscard]] const Vec3& GetSize() const;

	[[nodiscard]] AABB GetBoundingBox() const override;
	//bool CheckCollision(const ColliderComponent& other) const override;

private:
	TransformComponent* transform_ = nullptr;

	Vec3 size_ = Vec3::one;
};


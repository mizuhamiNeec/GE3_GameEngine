#pragma once

#include <math/public/MathLib.h>

#include <engine/public/Components/MeshRenderer/Base/MeshRenderer.h>

#include <engine/public/Animation/Animation.h>
#include <engine/public/renderer/ConstantBuffer.h>
#include <engine/public/ResourceSystem/Mesh/SkeletalMesh.h>

// ボーン変換行列用の定数バッファ構造体
struct BoneMatrices {
	static constexpr int MAX_BONES = 256;
	Mat4                 bones[MAX_BONES];
};

class SkeletalMeshRenderer : public MeshRenderer {
public:
	SkeletalMeshRenderer() = default;
	~SkeletalMeshRenderer() override;

	void OnAttach(Entity& owner) override;
	void Update(float deltaTime) override;

	// 描画処理
	void Render(ID3D12GraphicsCommandList* commandList) override;
	// インスペクターの描画
	void DrawInspectorImGui() override;

	[[nodiscard]] SkeletalMesh* GetSkeletalMesh() const;
	void                        SetSkeletalMesh(SkeletalMesh* skeletalMesh);

	// アニメーション制御
	void PlayAnimation(const std::string& animationName, bool loop = true);
	void StopAnimation();
	void PauseAnimation();
	void ResumeAnimation();
	void SetAnimationSpeed(float speed);
	[[nodiscard]] bool IsAnimationPlaying() const;
	[[nodiscard]] float GetAnimationTime() const;

protected:
	void BindTransform(ID3D12GraphicsCommandList* commandList) override;
	void UpdateBoneMatrices();
	void CalculateNodeTransform(const Node& node, const Mat4& parentTransform,
	                            const Animation* animation,
	                            float animationTime);

	void DrawBoneHierarchy(const Node& node, const Mat4& parentTransform);
	void DrawBoneDebug();

private:
	std::unique_ptr<ConstantBuffer> mTransformationMatrixConstantBuffer;
	TransformationMatrix*           mTransformationMatrix = nullptr;

	// ボーン変換行列用の定数バッファ
	std::unique_ptr<ConstantBuffer> mBoneMatricesConstantBuffer;
	BoneMatrices*                   mBoneMatrices = nullptr;

	SkeletalMesh* mSkeletalMesh = nullptr;

	// アニメーション制御
	const Animation* mCurrentAnimation = nullptr;
	std::string      mCurrentAnimationName;
	float            mAnimationTime  = 0.0f;
	float            mAnimationSpeed = 1.0f;
	bool             mIsPlaying      = false;
	bool             mIsLooping      = true;

#ifdef _DEBUG
	bool mShowBoneDebug = false;
#else
	bool mShowBoneDebug = false;
#endif

	// ライティング用の定数
	std::unique_ptr<ConstantBuffer> mMatParamCBV;
	MatParam*                       mMaterialData = nullptr;

	// b1
	std::unique_ptr<ConstantBuffer> mDirectionalLightCb;
	DirectionalLight*               mDirectionalLightData = nullptr;

	// b2
	std::unique_ptr<ConstantBuffer> mCameraCb;
	CameraForGPU*                   mCameraData = nullptr;

	// b3
	std::unique_ptr<ConstantBuffer> mPointLightCb;
	PointLight*                     mPointLightData = nullptr;

	// b4
	std::unique_ptr<ConstantBuffer> mSpotLightCb;
	SpotLight*                      mSpotLightData = nullptr;
};

#pragma once
#include <engine/public/renderer/SrvManager.h>
#include <engine/public/ResourceSystem/Animation/AnimationManager.h>
#include <engine/public/ResourceSystem/Material/MaterialManager.h>
#include <engine/public/ResourceSystem/Mesh/MeshManager.h>
#include <engine/public/ResourceSystem/Shader/ShaderManager.h>

class TexManager;

class ResourceManager {
public:
	ResourceManager(D3D12* d3d12);
	~ResourceManager() = default;

	void Init() const;
	void Shutdown();

	[[nodiscard]] SrvManager*       GetSrvManager() const;
	[[nodiscard]] TexManager*       GetTexManager() const;
	[[nodiscard]] ShaderManager*    GetShaderManager() const;
	[[nodiscard]] MaterialManager*  GetMaterialManager() const;
	[[nodiscard]] MeshManager*      GetMeshManager() const;
	[[nodiscard]] AnimationManager* GetAnimationManager() const;

private:
	D3D12* d3d12_;

	std::unique_ptr<SrvManager>       srvManager_;
	std::unique_ptr<ShaderManager>    shaderManager_;
	std::unique_ptr<MaterialManager>  materialManager_;
	std::unique_ptr<MeshManager>      meshManager_;
	std::unique_ptr<AnimationManager> animationManager_;
};

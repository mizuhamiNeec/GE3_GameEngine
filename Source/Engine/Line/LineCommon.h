﻿#pragma once
#include <memory>

#include "../Renderer/RootSignatureManager.h"
#include "../Renderer/PipelineState.h"

class Camera;
class D3D12;

class LineCommon {
public:
	void Init(D3D12* d3d12);
	void Shutdown() const;

	void CreateRootSignature();
	void CreateGraphicsPipeline();

	void Render();

	D3D12* GetD3D12() const;

	// Setter
	void SetDefaultCamera(Camera* camera);

	// Getter
	Camera* GetDefaultCamera() const;

private:
	Camera* defaultCamera_ = nullptr;
	D3D12* d3d12_ = nullptr;
	std::unique_ptr<RootSignatureManager> rootSignatureManager_ = nullptr;
	PipelineState pipelineState_;
};
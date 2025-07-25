#pragma once

#include <d3d12.h>
#include <dxcapi.h>
#include <string>
#include <wrl/client.h>

using namespace Microsoft::WRL;

enum BlendMode {
	//!< ブレンドなし
	kBlendModeNone = 0,
	kBlendModeNormal,   //!< 通常αブレンド Src * SrcA + Dest * (1 - SrcA)
	kBlendModeAdd,      //!< 加算 Src * SrcA + Desc * 1
	kBlendModeSubtract, //!< 減算 Desc * 1 - Src * SrcA
	kBlendModeMultiply, //!< 乗算 Src * 0 + Dest * Src
	kBlendModeScreen,   //!< スクリーン。 Src * (1 - Dest) + Desc * 1
	kCountOfBlendMode,  // ブレンドモード数
};

constexpr D3D12_DEPTH_STENCIL_DESC defaultDepthStencilDesc = {
	.DepthEnable = TRUE,                          // Depthの機能を有効化する
	.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL, // 書き込む
	.DepthFunc = D3D12_COMPARISON_FUNC_LESS,      // 比較関数はLess
	.StencilEnable = FALSE
};

class PipelineState {
public:
	PipelineState();
	PipelineState(D3D12_CULL_MODE cullMode, D3D12_FILL_MODE fillMode,
	              D3D12_PRIMITIVE_TOPOLOGY_TYPE topologyType,
	              const D3D12_DEPTH_STENCIL_DESC& depthStencilDesc =
		              defaultDepthStencilDesc);
	void             SetInputLayout(D3D12_INPUT_LAYOUT_DESC layout);
	void             SetRootSignature(ID3D12RootSignature* rootSignature);
	void             SetVertexShader(const std::wstring& filePath);
	void             SetPixelShader(const std::wstring& filePath);
	static IDxcBlob* CompileShader(
		const std::wstring& filePath, const wchar_t* profile,
		IDxcUtils*          dxcUtils,
		IDxcCompiler3*      dxcCompiler, IDxcIncludeHandler* includeHandler
	);
	void                    Create(ID3D12Device* device);
	void                    SetBlendMode(BlendMode blendMode);
	[[nodiscard]] BlendMode GetBlendMode() const;

	[[nodiscard]] ID3D12PipelineState* Get() const;
	void SetDepthWriteMask(D3D12_DEPTH_WRITE_MASK depthWriteMask);

private:
	D3D12_RASTERIZER_DESC mRasterizerDesc = {};

	D3D12_GRAPHICS_PIPELINE_STATE_DESC mDesc          = {};
	ComPtr<ID3D12PipelineState>        mPipelineState = nullptr;
	ComPtr<IDxcBlob>                   mVsBlob        = nullptr;
	ComPtr<IDxcBlob>                   mPsBlob        = nullptr;

	ComPtr<IDxcUtils>          mDxcUtils;
	ComPtr<IDxcCompiler3>      mDxcCompiler;
	ComPtr<IDxcIncludeHandler> mIncludeHandler;

	BlendMode mCurrentBlendMode = kBlendModeNone;
};

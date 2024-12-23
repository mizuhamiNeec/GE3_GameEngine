#pragma once

#include <d3d12.h>
#include <string>
#include <wrl/client.h>

#include "DirectXTex/DirectXTex.h"

class SrvManager;
class D3D12;
using namespace Microsoft::WRL;

class TextureManager {
public:
	static TextureManager* GetInstance();

	void Init(::D3D12* renderer, SrvManager* srvManager);
	static void Shutdown();

	void LoadTexture(const std::string& filePath);

	//static void UploadTextureData(const ComPtr<ID3D12Resource>& texture, const DirectX::ScratchImage& mipImages);
	[[nodiscard]] ComPtr<ID3D12Resource> UploadTextureData(
		const ComPtr<ID3D12Resource>& texture,
		const DirectX::ScratchImage& mipImages
	) const;

	// SRVインデックスの開始番号
	uint32_t GetTextureIndexByFilePath(const std::string& filePath) const;

	// テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(const std::string& filePath);

	[[nodiscard]] uint32_t GetLoadedTextureCount() const;

	const DirectX::TexMetadata& GetMetaData(const std::string& filePath) const;

private:
	[[nodiscard]] ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata) const;

private:
	// テクスチャ1枚分のデータ
	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	// テクスチャデータ
	std::unordered_map<std::string, TextureData> textureData_;

	D3D12* renderer_;
	SrvManager* srvManager_;

	static D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap,
	                                                          uint32_t descriptorSize, uint32_t index);
	static D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap,
	                                                          uint32_t descriptorSize, uint32_t index);

private:
	static TextureManager* instance_;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;
};

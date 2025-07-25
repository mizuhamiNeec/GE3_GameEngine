#pragma once
#include <cstdint>
#include <d3d12.h>
#include <vector>

#include <wrl/client.h>

class IndexBuffer {
public:
	IndexBuffer(const Microsoft::WRL::ComPtr<ID3D12Device>& device, size_t size, const void* pInitData);
	D3D12_INDEX_BUFFER_VIEW View();
	void Update(const void* pInitData, size_t size) const;
	size_t GetSize() const;
	std::vector<uint32_t>& GetIndices() const;

private:

	Microsoft::WRL::ComPtr<ID3D12Device> mDevice;
	Microsoft::WRL::ComPtr<ID3D12Resource> mBuffer;
	D3D12_INDEX_BUFFER_VIEW mView;

	size_t mSize = 0;

	mutable std::vector<uint32_t> mIndices;
};

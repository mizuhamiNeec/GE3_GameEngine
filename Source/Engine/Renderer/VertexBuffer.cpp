#include "VertexBuffer.h"

#include <cassert>

VertexBuffer::VertexBuffer(const ComPtr<ID3D12Device>& device, const size_t size, const size_t stride, const void* pInitData) {
	// リソース用のヒープを設定
	D3D12_HEAP_PROPERTIES uploadHeapProperties = {};
	uploadHeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD; // UploadHeapを使う

	D3D12_RESOURCE_DESC resourceDesc = {};

	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = size; // リソースのサイズ
	// バッファの場合はこれらは1にする決まり
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	// バッファの場合はこれにする決まり
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	// 実際にリソースを作る
	HRESULT hr = device->CreateCommittedResource(
		&uploadHeapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(buffer_.GetAddressOf())
	);

	assert(SUCCEEDED(hr));

	view_.BufferLocation = buffer_->GetGPUVirtualAddress();
	view_.SizeInBytes = static_cast<UINT>(size);
	view_.StrideInBytes = static_cast<UINT>(stride);

	if (pInitData != nullptr) {
		void* ptr = nullptr;
		hr = buffer_->Map(0, nullptr, &ptr);
		assert(SUCCEEDED(hr));

		memcpy(ptr, pInitData, size);
		buffer_->Unmap(0, nullptr);
	}
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::View() const {
	return view_;
}

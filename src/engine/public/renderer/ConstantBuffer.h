#pragma once

#include <d3d12.h>
#include <xstring>
#include <wrl/client.h>

class ConstantBuffer {
public:
	ConstantBuffer(const Microsoft::WRL::ComPtr<ID3D12Device>& device,
	               size_t size, std::string name);
	~ConstantBuffer();
	D3D12_GPU_VIRTUAL_ADDRESS       GetAddress() const;
	D3D12_CONSTANT_BUFFER_VIEW_DESC ViewDesc() const;

	void* GetPtr() const;

	ID3D12Resource* GetResource() const;

	template <typename T>
	T* GetPtr() {
		return static_cast<T*>(GetPtr());
	}

private:
	Microsoft::WRL::ComPtr<ID3D12Resource> mBuffer;
	D3D12_CONSTANT_BUFFER_VIEW_DESC        mDesc;
	void*                                  mAppedPtr = nullptr;
	std::string                            mName;

	ConstantBuffer(const ConstantBuffer&) = delete;
	void operator=(const ConstantBuffer&) = delete;
};

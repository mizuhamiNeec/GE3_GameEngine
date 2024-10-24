#include "D3D12.h"

#include <cassert>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <dxgidebug.h>
#include <format>

#include "../Utils/ConvertString.h"
#include "../Utils/Logger.h"
#include "DirectXTex/d3dx12.h"
#include <dxgi1_3.h>
#include "../Utils/ClientProperties.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "dxguid.lib")
#pragma comment(lib, "dxcompiler.lib")

void D3D12::Initialize(Window* window) {
	window_ = window;
	windowConfig_ = window->GetWindowConfig();

#ifdef _DEBUG
	EnableDebugLayer();
#endif

	CreateDevice();

#ifdef _DEBUG
	SetInfoQueueBreakOnSeverity();
#endif

	CreateCommandQueue();
	CreateSwapChain();
	CreateDescriptorHeaps();
	CreateRTV();
	CreateDSV();
	CreateCommandAllocator();
	CreateCommandList();
	CreateFence();

	SetViewportAndScissor();

	Log("Complete Initialize DirectX12.\n");
}

void D3D12::ClearColorAndDepth() const {
	float clearColor[] = {0.89f, 0.5f, 0.03f, 1.0f};
	commandList_->ClearRenderTargetView(
		rtvHandles_[frameIndex_],
		clearColor,
		0,
		nullptr
	);

	D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = GetCPUDescriptorHandle(
		dsvDescriptorHeap_.Get(),
		descriptorSizeDSV,
		0
	);

	commandList_->ClearDepthStencilView(
		dsvHandle,
		D3D12_CLEAR_FLAG_DEPTH,
		1.0f,
		0,
		0,
		nullptr
	);

	commandList_->OMSetRenderTargets(
		1,
		&rtvHandles_[frameIndex_],
		false,
		&dsvHandle
	);
}

void D3D12::PreRender() {
	//-------------------------------------------------------------------------
	// これから書き込むバックバッファのインデックスを取得
	//-------------------------------------------------------------------------
	frameIndex_ = swapChain_->GetCurrentBackBufferIndex();

	//-------------------------------------------------------------------------
	// コマンドのリセット
	//-------------------------------------------------------------------------
	HRESULT hr = commandAllocator_->Reset();
	assert(SUCCEEDED(hr));
	hr = commandList_->Reset(
		commandAllocator_.Get(),
		nullptr
	);
	assert(SUCCEEDED(hr));

	//-----------------------------------------------------------------------------
	// ビューポートとシザー矩形を設定
	//-----------------------------------------------------------------------------
	commandList_->RSSetViewports(1, &viewport_);
	commandList_->RSSetScissorRects(1, &scissorRect_);

	//-------------------------------------------------------------------------
	// リソースバリアを張る
	//-------------------------------------------------------------------------
	barrier_.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier_.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	barrier_.Transition.pResource = renderTargets_[frameIndex_].Get();
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	commandList_->ResourceBarrier(1, &barrier_);

	//-------------------------------------------------------------------------
	// レンダーターゲットと深度ステンシルバッファをクリアする
	//-------------------------------------------------------------------------
	ClearColorAndDepth();
}

void D3D12::PostRender() {
	//-------------------------------------------------------------------------
	// リソースバリア遷移↓
	//-------------------------------------------------------------------------
	barrier_.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrier_.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	commandList_->ResourceBarrier(1, &barrier_);

	//-------------------------------------------------------------------------
	// コマンドリストを閉じる
	//-------------------------------------------------------------------------
	const HRESULT hr = commandList_->Close();
	assert(SUCCEEDED(hr));

	//-------------------------------------------------------------------------
	// コマンドのキック
	//-------------------------------------------------------------------------
	ID3D12CommandList* lists[] = {commandList_.Get()};
	commandQueue_->ExecuteCommandLists(1, lists);

	//-------------------------------------------------------------------------
	// GPU と OS に画面の交換を行うよう通知
	//-------------------------------------------------------------------------
	swapChain_->Present(1, 0);

	WaitPreviousFrame(); // 前のフレームを待つ
}

void D3D12::Terminate() {
	CloseHandle(fenceEvent_);
	CloseWindow(window_->GetHWND());

	Log("Bye!\n");
}

void D3D12::OnSizeChanged(UINT width, UINT height, bool isMinimized) {
	width; height; isMinimized;
}

void D3D12::ToggleFullscreen() {
}

void D3D12::WriteToUploadHeapMemory(ID3D12Resource* resource, uint32_t size, const void* data) {
	void* mapped;
	HRESULT hr = resource->Map(0, nullptr, &mapped);
	if (SUCCEEDED(hr)) {
		memcpy(mapped, data, size);
		resource->Unmap(0, nullptr);
	}
}

void D3D12::EnableDebugLayer() {
	ID3D12Debug6* debugController;

	if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController)))) {
		debugController->EnableDebugLayer(); //デバッグレイヤーの有効化
		debugController->SetEnableGPUBasedValidation(TRUE); // GPU側でのチェックを有効化
	}
}

void D3D12::CreateDevice() {
	//-------------------------------------------------------------------------
	// ファクトリーの生成
	//-------------------------------------------------------------------------
	HRESULT hr;
	UINT dxgiFlags = 0;

#ifdef _DEBUG
	// デバッグ有効化時にはDXGI_CREATE_FACTORY_DEBUG フラグを付与する
	dxgiFlags |= DXGI_CREATE_FACTORY_DEBUG;
#endif

	dxgiFlags = 0;

	hr = CreateDXGIFactory2(dxgiFlags, IID_PPV_ARGS(&dxgiFactory_));
	assert(SUCCEEDED(hr));

	//-------------------------------------------------------------------------
	// ハードウェアアダプタの検索
	//-------------------------------------------------------------------------
	ComPtr<IDXGIAdapter4> useAdapter;
	for (UINT i = 0; dxgiFactory_->EnumAdapterByGpuPreference(i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE,
		IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {
		DXGI_ADAPTER_DESC3 adapterDesc;
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));
		// ソフトウェアアダプタでなければ採用
		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {
			// 採用したアダプタの情報をログに出力
			Log(ConvertString(std::format(L"Use Adapter : {}\n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr; // ソフトウェアアダプタの場合は見なかったことにする
	}
	assert(useAdapter != nullptr); // 適切なアダプタが見つからなかったので起動できない

	//-------------------------------------------------------------------------
	// D3D12デバイスの生成
	//-------------------------------------------------------------------------
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2, D3D_FEATURE_LEVEL_12_1, D3D_FEATURE_LEVEL_12_0,
		D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0,
	};
	const char* featureLevelStrings[] = {"12.2", "12.1", "12.0", "11.1", "11.0"};

	// 高い順に生成できるか試していく
	for (size_t i = 0; i < _countof(featureLevels); ++i) {
		// 採用したアダプターでデバイスを生成
		hr = D3D12CreateDevice(useAdapter.Get(), featureLevels[i], IID_PPV_ARGS(&device_));
		// 指定した機能レベルでデバイスが生成できたをできたかを確認
		if (SUCCEEDED(hr)) {
			// 生成できたのでログに出力し、ループを抜ける
			Log(std::format("FeatureLevel : {}\n", featureLevelStrings[i]));
			break;
		}
		device_ = nullptr;
	}
	assert(device_ != nullptr); // デバイスの生成がうまくいかなかったので起動できない

	Log("Complete create D3D12Device.\n"); // 初期化完了のログを出す
}

void D3D12::SetInfoQueueBreakOnSeverity() const {
	ComPtr<ID3D12InfoQueue> infoQueue;
	if (SUCCEEDED(device_->QueryInterface(IID_PPV_ARGS(&infoQueue)))) {
		// やばいエラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
		// エラー時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
		// 警告時に止まる
		infoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);

		// 抑制するメッセージのID
		D3D12_MESSAGE_ID denyIds[] = {
			// Windows11でのDXGIデバッグレイヤーとDX12デバッグレイヤーの相互作用バグによるエラーメッセージ
			// https://stackoverflow.com/questions/69805245/directx-12-application-is-crashing-in-windows-11
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE
		};

		// 抑制するレベル
		D3D12_MESSAGE_SEVERITY severities[] = {D3D12_MESSAGE_SEVERITY_INFO};
		D3D12_INFO_QUEUE_FILTER filter = {};
		filter.DenyList.NumIDs = _countof(denyIds);
		filter.DenyList.pIDList = denyIds;
		filter.DenyList.NumSeverities = _countof(severities);
		filter.DenyList.pSeverityList = severities;
		// 指定したメッセージの表示を抑制する
		infoQueue->PushStorageFilter(&filter);
	}
}

void D3D12::CreateCommandQueue() {
	//------------------------------------------------------------------------
	// コマンドキューの生成
	//------------------------------------------------------------------------
	constexpr D3D12_COMMAND_QUEUE_DESC commandQueueDesc = {
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		0,
		D3D12_COMMAND_QUEUE_FLAG_NONE,
		0
	};
	const HRESULT hr = device_->CreateCommandQueue(&commandQueueDesc, IID_PPV_ARGS(&commandQueue_));
	assert(SUCCEEDED(hr));
	Log("Complete create CommandQueue.\n");
}

void D3D12::CreateSwapChain() {
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.BufferCount = kFrameBufferCount;
	swapChainDesc.Width = windowConfig_.clientWidth;
	swapChainDesc.Height = windowConfig_.clientHeight;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 色の形式
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // レンダーターゲットとして利用
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // モニタに映したら中身を破棄
	swapChainDesc.SampleDesc.Count = 1; // マルチサンプルしない
	swapChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

	const HRESULT hr = dxgiFactory_->CreateSwapChainForHwnd(
		commandQueue_.Get(),
		window_->GetHWND(),
		&swapChainDesc,
		nullptr,
		nullptr,
		reinterpret_cast<IDXGISwapChain1**>(swapChain_.GetAddressOf())
	);
	assert(SUCCEEDED(hr));
	Log("Complete create SwapChain.\n");
}

void D3D12::CreateDescriptorHeaps() {
	//-------------------------------------------------------------------
	// DescriptorSizeを取得しておく
	//-------------------------------------------------------------------
	descriptorSizeSRV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	descriptorSizeRTV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	descriptorSizeDSV = device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);


	// SRV用のヒープでディスクリプタの数は128。SRVはShader内で触るものなので、ShaderVisibleはtrue
	srvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 128, true);
	// RTV用のヒープでディスクリプタの数は2。RTVはShader内で触るものではないので、ShaderVisibleはfalse
	rtvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, kFrameBufferCount, false);
	// DSV用のヒープでディスクリプタの数は1。DSVはShader内で触るものではないので、ShaderVisibleはfalse
	dsvDescriptorHeap_ = CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_DSV, 1, false);
}

void D3D12::CreateRTV() {
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {}; // RTVの設定
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // 出力結果を書き込む
	rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D; // 2Dテクスチャとして書き込む

	renderTargets_.resize(kFrameBufferCount); // フレームバッファ数でリサイズ
	rtvHandles_.resize(kFrameBufferCount);

	for (unsigned int i = 0; i < kFrameBufferCount; ++i) {
		// SwapChainからResourceを引っ張ってくる
		const HRESULT hr = swapChain_->GetBuffer(i, IID_PPV_ARGS(&renderTargets_[i]));
		assert(SUCCEEDED(hr));

		rtvHandles_[i] = GetCPUDescriptorHandle(rtvDescriptorHeap_.Get(), descriptorSizeRTV, i);
		device_->CreateRenderTargetView(renderTargets_[i].Get(), &rtvDesc, rtvHandles_[i]);
	}
	Log("Complete create RenderTargetView.\n");
}

void D3D12::CreateDSV() {
	depthStencilResource_ = CreateDepthStencilTextureResource();

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
	dsvDesc.Format = depthStencilResource_->GetDesc().Format;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	device_->CreateDepthStencilView(depthStencilResource_.Get(), &dsvDesc,
		dsvDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
}

void D3D12::CreateCommandAllocator() {
	const HRESULT hr = device_->
		CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&commandAllocator_));
	assert(SUCCEEDED(hr));
	Log("Complete create CommandAllocator.\n");
}

void D3D12::CreateCommandList() {
	const HRESULT hr = device_->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocator_.Get(), nullptr,
		IID_PPV_ARGS(&commandList_));
	assert(SUCCEEDED(hr));
	commandList_->Close();
	Log("Complete create CommandList.\n");
}

void D3D12::CreateFence() {
	const HRESULT hr = device_->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence_));
	assert(SUCCEEDED(hr));

	fenceEvent_ = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	assert(fenceEvent_ != nullptr);
	Log("Complete create Fence.\n");
}

void D3D12::SetViewportAndScissor() {
	// Viewport
	viewport_.TopLeftX = 0;
	viewport_.TopLeftY = 0;
	viewport_.Width = static_cast<FLOAT>(windowConfig_.clientWidth);
	viewport_.Height = static_cast<FLOAT>(windowConfig_.clientHeight);
	viewport_.MinDepth = 0.0f;
	viewport_.MaxDepth = 1.0f;

	// ScissorRect
	scissorRect_.left = 0;
	scissorRect_.top = 0;
	scissorRect_.right = static_cast<LONG>(windowConfig_.clientWidth);
	scissorRect_.bottom = static_cast<LONG>(windowConfig_.clientHeight);
}

void D3D12::WaitPreviousFrame() {
	fenceValue_++; // Fenceの値を更新

	// GPUがここまでたどり着いたときに、Fenceの値を指定した値に代入するようにSignalを送る
	commandQueue_->Signal(fence_.Get(), fenceValue_);

	// Fenceの値が指定したSignal値にたどり着いているか確認する
	// GetCompletedValueの初期値はFence作成時に渡した初期値
	if (fence_->GetCompletedValue() < fenceValue_) {
		// 指定したSignalにたどり着いていないので、たどり着くまで待つようにイベントを設定する
		fence_->SetEventOnCompletion(fenceValue_, fenceEvent_);
		// イベント待つ
		WaitForSingleObject(fenceEvent_, INFINITE);
	}
}

ComPtr<ID3D12DescriptorHeap> D3D12::CreateDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_TYPE heapType, const UINT numDescriptors, const bool shaderVisible) const {
	ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = heapType;
	descriptorHeapDesc.NumDescriptors = numDescriptors;
	if (shaderVisible) {
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	} else {
		descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	}
	const HRESULT hr = device_->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap));
	assert(SUCCEEDED(hr));
	return descriptorHeap;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12::GetCPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, const uint32_t descriptorSize, const uint32_t index) {
	D3D12_CPU_DESCRIPTOR_HANDLE handleCPU = descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	handleCPU.ptr += static_cast<unsigned long long>(descriptorSize) * index;
	return handleCPU;
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12::GetGPUDescriptorHandle(ID3D12DescriptorHeap* descriptorHeap, const uint32_t descriptorSize, const uint32_t index) {
	D3D12_GPU_DESCRIPTOR_HANDLE handleGPU = descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handleGPU.ptr += static_cast<unsigned long long>(descriptorSize) * index;
	return handleGPU;
}

ComPtr<ID3D12Resource> D3D12::CreateDepthStencilTextureResource() const {
	// 生成するResourceの設定
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Width = windowConfig_.clientWidth;
	resourceDesc.Height = windowConfig_.clientHeight;
	resourceDesc.MipLevels = 1; // Mipmapの数
	resourceDesc.DepthOrArraySize = 1; // 奥行きor配列Textureの配列数
	resourceDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT; // DepthStencilとして利用可能なフォーマット
	resourceDesc.SampleDesc.Count = 1; // サンプリングカウント。1固定
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D; // 2次元
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL; // DepthStencilとして使う通知

	D3D12_HEAP_PROPERTIES heapProperties = {}; // 利用するHeapの設定
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT; // vRAM上に作る

	// 深度値のクリア設定
	D3D12_CLEAR_VALUE depthClearValue = {};
	depthClearValue.DepthStencil.Depth = 1.0f; // 最大値でクリア
	depthClearValue.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;

	// Resourceの生成
	ComPtr<ID3D12Resource> resource;
	const HRESULT hr = device_->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&depthClearValue,
		IID_PPV_ARGS(&resource)
	); // 作成するResourceポインタへのポインタ
	assert(SUCCEEDED(hr));

	return resource;
}

D3DResourceLeakChecker::~D3DResourceLeakChecker() {
	// リソースリークチェック
	ComPtr<IDXGIDebug> debug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debug)))) {
		debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_APP, DXGI_DEBUG_RLO_ALL);
		debug->ReportLiveObjects(DXGI_DEBUG_D3D12, DXGI_DEBUG_RLO_ALL);
	}
}

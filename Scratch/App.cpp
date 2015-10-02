#include "pch.h"
#include "App.h"
#include "moti/windows/utility.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Windows::UI::Core;
using namespace Microsoft::WRL;
using namespace moti::windows::utility;

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto app = ref new Scratch::AppViewSource();
	CoreApplication::Run(app);
	return 0;
}

void Scratch::App::Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^ApplicationView)
{
	window_ = ApplicationView->CoreWindow;
}

void Scratch::App::SetWindow(Windows::UI::Core::CoreWindow ^window)
{
	window_ = window;

	// TODO: handle window changed
}

void Scratch::App::Load(Platform::String ^entryPoint)
{
	// enable debug layer
#if defined(_DEBUG)
	{
		ComPtr<ID3D12Debug> debug_controller;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug_controller)))) {
			debug_controller->EnableDebugLayer();
		}
	}
#endif

	// create DXGI Factory
	{
		ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory_)));
	}

	// create D3D12 device
	{
		ThrowIfFailed(
			D3D12CreateDevice(
				nullptr,					// use default adapter
				D3D_FEATURE_LEVEL_12_1,
				IID_PPV_ARGS(&d3d_device_)
			)
		);
	}

	// create command queue
	{
		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ThrowIfFailed(d3d_device_->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue_)));
	}

	// create command allocator
	{
		ComPtr<ID3D12CommandAllocator> command_allocators[num_frames_];
		for (UINT n = 0; n < num_frames_; n++) {
			ThrowIfFailed(
				d3d_device_->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(&command_allocators[n])
				)
			);
		}
	}

	// get size of window
	{
		// TODO:
		// - DPI
		// - Orientation
		// assume window is valid
		assert(!(window_ == nullptr));
		Windows::Foundation::Size logical_size{ window_->Bounds.Width, window_->Bounds.Height };

		// create swap chain
		{
			DXGI_SWAP_CHAIN_DESC1 swap_chain_desc = {};
			swap_chain_desc.Width = lround(logical_size.Width);
			swap_chain_desc.Height = lround(logical_size.Height);
			swap_chain_desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
			swap_chain_desc.Stereo = false;
			swap_chain_desc.SampleDesc.Count = 1;		// Don't use multi-sampling
			swap_chain_desc.SampleDesc.Quality = 0;
			swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			swap_chain_desc.BufferCount = num_frames_;
			swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			swap_chain_desc.Flags = 0;
			swap_chain_desc.Scaling = DXGI_SCALING_NONE;
			swap_chain_desc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

			ComPtr<IDXGISwapChain1> swap_chain;
			ThrowIfFailed(
				dxgi_factory_->CreateSwapChainForCoreWindow(
					command_queue_.Get(),
					reinterpret_cast<IUnknown*>(window_.Get()),
					&swap_chain_desc,
					nullptr,
					&swap_chain
					)
				);

			ThrowIfFailed(swap_chain.As(&swap_chain_));
		}
	}

	// create render target view
	{
		D3D12_DESCRIPTOR_HEAP_DESC render_target_view_heap_desc = {};
		render_target_view_heap_desc.NumDescriptors = num_frames_;
		render_target_view_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		render_target_view_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
		ThrowIfFailed(
			d3d_device_->CreateDescriptorHeap(
				&render_target_view_heap_desc,
				IID_PPV_ARGS(&render_target_view_heap_)
				)
			);
		render_target_view_heap_->SetName(L"Render Target View Descriptor Heap");

		D3D12_CPU_DESCRIPTOR_HANDLE render_target_view_desc =
			render_target_view_heap_->GetCPUDescriptorHandleForHeapStart();

		auto render_target_view_desc_size_ =
			d3d_device_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		for (UINT n = 0; n < num_frames_; n++) {
			ThrowIfFailed(swap_chain_->GetBuffer(n, IID_PPV_ARGS(&render_targets_[n])));
			d3d_device_->CreateRenderTargetView(
				render_targets_[n].Get(),
				nullptr,
				render_target_view_desc);

			render_target_view_desc.ptr += render_target_view_desc_size_;

			WCHAR name[25] = L"";
			swprintf_s(name, L"Render Target %d", n);
			render_targets_[n]->SetName(name);
		}
	}

	// create synchronization objects
	{
		current_frame_ = swap_chain_->GetCurrentBackBufferIndex();
		{
			ZeroMemory(fence_values_, sizeof(fence_values_));
			ComPtr<ID3D12Fence> fence;
			ThrowIfFailed(
				d3d_device_->CreateFence(
					fence_values_[current_frame_],
					D3D12_FENCE_FLAG_NONE,
					IID_PPV_ARGS(&fence_))
				);
			fence_values_[current_frame_]++;
		}
		{
			HANDLE fence_event = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		}
	}

}

void Scratch::App::Run()
{
	while (true) {
		CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);
	}
}

void Scratch::App::Uninitialize()
{
}

Windows::ApplicationModel::Core::IFrameworkView ^ Scratch::AppViewSource::CreateView()
{
	return ref new App();
}

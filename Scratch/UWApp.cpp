#include "pch.h"
#include "UWApp.h"
#include "moti/windows/utility.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;
using namespace Microsoft::WRL;
using namespace moti::windows::utility;

[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto UWApp = ref new Scratch::UWAppViewSource();
	CoreApplication::Run(UWApp);
	return 0;
}

void Scratch::UWApp::Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^ApplicationView)
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

	// create DXGIFactory
	ComPtr<IDXGIFactory1> dxgi_factory;
	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)));

	// create D3D12 device
	ComPtr<ID3D12Device> d3d_device;
	ThrowIfFailed(
		D3D12CreateDevice(
			nullptr,					// use default adapter
			D3D_FEATURE_LEVEL_12_0,
			IID_PPV_ARGS(&d3d_device)
			)
		);

	// create command queue
	ComPtr<ID3D12CommandQueue> command_queue;
	D3D12_COMMAND_QUEUE_DESC queue_desc = {};
	queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	ThrowIfFailed(d3d_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&command_queue)));

	constexpr UINT num_frames = 3; // use triple buffering
	ComPtr<ID3D12CommandAllocator> command_allocators[num_frames];
	for (UINT n = 0; n < num_frames; n++) {
		ThrowIfFailed(
			d3d_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&command_allocators[n]))
			);
	}

	// create synchronization objects
	UINT current_frame = 0;
	UINT64 fence_values[num_frames];
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(
		d3d_device->CreateFence(
			fence_values[current_frame],
			D3D12_FENCE_FLAG_NONE,
			IID_PPV_ARGS(&fence))
		);
	fence_values[current_frame]++;

	HANDLE fence_event = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
}

void Scratch::UWApp::SetWindow(Windows::UI::Core::CoreWindow ^window)
{
}

void Scratch::UWApp::Load(Platform::String ^entryPoint)
{
}

void Scratch::UWApp::Run()
{
}

void Scratch::UWApp::Uninitialize()
{
}

Windows::ApplicationModel::Core::IFrameworkView ^ Scratch::UWAppViewSource::CreateView()
{
	return ref new UWApp();
}

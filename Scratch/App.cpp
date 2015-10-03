#include "pch.h"
#include "App.h"
#include "moti/windows/utility.h"

using namespace Windows::Foundation;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
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

void Scratch::App::Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^applicationView)
{
	window_ = applicationView->CoreWindow;

	applicationView->Activated +=
		ref new TypedEventHandler<CoreApplicationView^, IActivatedEventArgs^>(this, &App::OnActivated);
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
		for (UINT n = 0; n < num_frames_; n++) {
			ThrowIfFailed(
				d3d_device_->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(&command_allocators_[n])
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

		render_target_view_desc_size_ =
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
			fence_event_ = CreateEventEx(nullptr, FALSE, FALSE, EVENT_ALL_ACCESS);
		}
	}

	// create graphics command list
	{
		ThrowIfFailed(
			d3d_device_->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				command_allocators_[current_frame_].Get(),
				nullptr,
				IID_PPV_ARGS(&command_list_)
			)
		);
	}

	// Nothing to record on load.
	ThrowIfFailed(command_list_->Close());

	LoadAssets();
}

void Scratch::App::LoadAssets()
{
	// Create an empty root signature
	{
		D3D12_ROOT_SIGNATURE_DESC root_signature_desc;
		root_signature_desc.NumParameters = 0;
		root_signature_desc.pParameters = nullptr;
		root_signature_desc.NumStaticSamplers = 0;
		root_signature_desc.pStaticSamplers = nullptr;
		root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ComPtr<ID3DBlob> signature;
		ComPtr<ID3DBlob> error;
		ThrowIfFailed(
			D3D12SerializeRootSignature(
				&root_signature_desc,
				D3D_ROOT_SIGNATURE_VERSION_1,
				&signature,
				&error
			)
		);
		ThrowIfFailed(
			d3d_device_->CreateRootSignature(
				0,
				signature->GetBufferPointer(),
				signature->GetBufferSize(),
				IID_PPV_ARGS(&root_signature_)
			)
		);
	}
}

void Scratch::App::OnActivated(CoreApplicationView ^ applicationView, IActivatedEventArgs ^ args)
{
	// need to activate to dismiss splash icon
	CoreWindow::GetForCurrentThread()->Activate();
}

void Scratch::App::Render()
{
	// assume that command allocator of "current frame" is executed.
	{
		ThrowIfFailed(command_allocators_[current_frame_]->Reset());
		ThrowIfFailed(command_list_->Reset(command_allocators_[current_frame_].Get(), pipeline_state_.Get()));
	}

	// Indicate that the back buffer will be used as a render target.
	{
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = render_targets_[current_frame_].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		command_list_->ResourceBarrier(1, &barrier);
	}

	D3D12_CPU_DESCRIPTOR_HANDLE render_target_view_handle = { render_target_view_heap_->GetCPUDescriptorHandleForHeapStart().ptr + current_frame_ * render_target_view_desc_size_ };

	const float clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
	command_list_->ClearRenderTargetView(render_target_view_handle, clear_color, 0, nullptr);

	// Indicate that the back buffer will now be used to present
	{
		D3D12_RESOURCE_BARRIER barrier;
		barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		barrier.Transition.pResource = render_targets_[current_frame_].Get();
		barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		command_list_->ResourceBarrier(1, &barrier);
	}

	// Finish command list population
	ThrowIfFailed(command_list_->Close());

	// Execute command list
	ID3D12CommandList* command_lists[] = { command_list_.Get() };
	command_queue_->ExecuteCommandLists(_countof(command_lists), command_lists);

	// Present this frame.
	ThrowIfFailed(swap_chain_->Present(1, 0));

	// Wait for previous frame
	{
		const auto fence = fence_values_[current_frame_];
		ThrowIfFailed(command_queue_->Signal(fence_.Get(), fence));
		fence_values_[current_frame_]++;

		// wait until the previous frame is finished
		if (fence_->GetCompletedValue() < fence) {
			ThrowIfFailed(fence_->SetEventOnCompletion(fence, fence_event_));
			WaitForSingleObject(fence_event_, INFINITE);
		}
		current_frame_ = swap_chain_->GetCurrentBackBufferIndex();
	}
}

void Scratch::App::Run()
{
	while (true) {
		CoreWindow::GetForCurrentThread()->Dispatcher->ProcessEvents(CoreProcessEventsOption::ProcessAllIfPresent);

		Render();
	}
}

void Scratch::App::Uninitialize()
{
}

Windows::ApplicationModel::Core::IFrameworkView ^ Scratch::AppViewSource::CreateView()
{
	return ref new App();
}

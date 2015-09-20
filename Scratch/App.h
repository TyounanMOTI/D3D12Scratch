#pragma once

#include <pch.h>

namespace Scratch {
	ref class App sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		// Inherited via IFrameworkView
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^ApplicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow ^window);
		virtual void Load(Platform::String ^entryPoint);
		virtual void Run();
		virtual void Uninitialize();

		void CreateDeviceResources();
		void CreateWindowDependentResources();

	private:
		static const UINT num_frames_ = 3; // use triple buffering
		UINT current_frame_;
		UINT64 fence_values_[num_frames_];

		Platform::Agile<Windows::UI::Core::CoreWindow> window_;
		Microsoft::WRL::ComPtr<ID3D12Device> d3d_device_;
		Microsoft::WRL::ComPtr<IDXGIFactory4> dxgi_factory_;
		Microsoft::WRL::ComPtr<ID3D12CommandQueue> command_queue_;
		Microsoft::WRL::ComPtr<ID3D12Fence> fence_;
		Microsoft::WRL::ComPtr<IDXGISwapChain3> swap_chain_;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> render_target_view_heap_;
		Microsoft::WRL::ComPtr<ID3D12Resource> render_targets_[num_frames_];
	};

	ref class AppViewSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
	{
	public:
		// Inherited via IFrameworkViewSource
		virtual Windows::ApplicationModel::Core::IFrameworkView ^ CreateView();
	};
}

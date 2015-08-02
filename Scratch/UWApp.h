#pragma once

namespace Scratch {
	ref class UWApp sealed : public Windows::ApplicationModel::Core::IFrameworkView
	{
	public:
		// Inherited via IFrameworkView
		virtual void Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^ApplicationView);
		virtual void SetWindow(Windows::UI::Core::CoreWindow ^window);
		virtual void Load(Platform::String ^entryPoint);
		virtual void Run();
		virtual void Uninitialize();
	};

	ref class UWAppViewSource sealed : Windows::ApplicationModel::Core::IFrameworkViewSource
	{
	public:
		// Inherited via IFrameworkViewSource
		virtual Windows::ApplicationModel::Core::IFrameworkView ^ CreateView();
	};
}

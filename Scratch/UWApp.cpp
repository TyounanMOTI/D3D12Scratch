#include "pch.h"
#include "UWApp.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;


[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto UWApp = ref new Scratch::UWAppViewSource();
	CoreApplication::Run(UWApp);
	return 0;
}

void Scratch::UWApp::Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^ApplicationView)
{
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

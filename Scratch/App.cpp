#include "pch.h"
#include "App.h"

using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Core;


[Platform::MTAThread]
int main(Platform::Array<Platform::String^>^)
{
	auto app = ref new Scratch::AppViewSource();
	CoreApplication::Run(app);
	return 0;
}

void Scratch::App::Initialize(Windows::ApplicationModel::Core::CoreApplicationView ^applicationView)
{
}

void Scratch::App::SetWindow(Windows::UI::Core::CoreWindow ^window)
{
}

void Scratch::App::Load(Platform::String ^entryPoint)
{
}

void Scratch::App::Run()
{
}

void Scratch::App::Uninitialize()
{
}

Windows::ApplicationModel::Core::IFrameworkView ^ Scratch::AppViewSource::CreateView()
{
	return ref new App();
}

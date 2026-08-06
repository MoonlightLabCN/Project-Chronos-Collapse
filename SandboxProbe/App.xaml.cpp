#include "pch.h"
#include "MainPage.xaml.h"

using namespace SandboxProbe;

using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::ApplicationModel::Activation;
using namespace Windows::Foundation;
using namespace Windows::Foundation::Collections;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Controls::Primitives;
using namespace Windows::UI::Xaml::Data;
using namespace Windows::UI::Xaml::Input;
using namespace Windows::UI::Xaml::Interop;
using namespace Windows::UI::Xaml::Media;
using namespace Windows::UI::Xaml::Navigation;

App::App()
{
    InitializeComponent();
}

void App::OnLaunched(LaunchActivatedEventArgs^ e)
{
    auto rootFrame = dynamic_cast<Frame^>(Window::Current->Content);

    if (rootFrame == nullptr)
    {
        rootFrame = ref new Frame();
        rootFrame->NavigationFailed += ref new NavigationFailedEventHandler(this, &App::OnNavigationFailed);

        if (e->PreviousExecutionState == ApplicationExecutionState::Terminated)
        {
            // TODO: Load state from previously suspended application
        }

        Window::Current->Content = rootFrame;
    }

    if (e->PrelaunchActivated == false)
    {
        if (rootFrame->Content == nullptr)
        {
            rootFrame->Navigate(TypeName(MainPage::typeid), e->Arguments);
        }
        Window::Current->Activate();
    }
}

void App::OnNavigationFailed(Object^ sender, NavigationFailedEventArgs^ e)
{
    throw ref new FailureException("Failed to load Page " + e->SourcePageType.Name);
}

// Generated code from App.g.hpp
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
extern "C" __declspec(dllimport) int __stdcall IsDebuggerPresent();
#endif

void SandboxProbe::App::InitializeComponent()
{
#if defined _DEBUG && !defined DISABLE_XAML_GENERATED_BREAK_ON_UNHANDLED_EXCEPTION
    UnhandledException += ref new Windows::UI::Xaml::UnhandledExceptionEventHandler(
        [](Platform::Object^ sender, Windows::UI::Xaml::UnhandledExceptionEventArgs^ e)
        {
            (void)sender;
            if (IsDebuggerPresent())
            {
                Platform::String^ errorMessage = e->Message;
                __debugbreak();
            }
        });
#endif
}

Windows::UI::Xaml::Markup::IXamlType^ SandboxProbe::App::GetXamlType(Windows::UI::Xaml::Interop::TypeName type)
{
    return _AppProvider->GetXamlTypeByType(type);
}

Windows::UI::Xaml::Markup::IXamlType^ SandboxProbe::App::GetXamlType(Platform::String^ fullName)
{
    return _AppProvider->GetXamlTypeByName(fullName);
}

Platform::Array<Windows::UI::Xaml::Markup::XmlnsDefinition>^ SandboxProbe::App::GetXmlnsDefinitions()
{
    return ref new Platform::Array<Windows::UI::Xaml::Markup::XmlnsDefinition>(0);
}

XamlTypeInfo::InfoProvider::XamlTypeInfoProvider^ SandboxProbe::App::_AppProvider::get()
{
    if (__provider == nullptr)
    {
        __provider = ref new XamlTypeInfo::InfoProvider::XamlTypeInfoProvider();
    }
    return __provider;
}

#ifndef DISABLE_XAML_GENERATED_MAIN
int __cdecl main(Platform::Array<Platform::String^>^ args)
{
    (void)args;
    Windows::UI::Xaml::Application::Start(ref new Windows::UI::Xaml::ApplicationInitializationCallback(
        [](Windows::UI::Xaml::ApplicationInitializationCallbackParams^ p) {
            (void)p;
            auto app = ref new SandboxProbe::App();
        }));
    return 0;
}
#endif

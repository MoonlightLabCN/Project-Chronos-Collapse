#pragma once

#include "MainPage.g.h"

namespace SandboxProbe
{
    public ref class MainPage sealed
    {
    public:
        MainPage();

        void OnOpenPackage(Platform::Object^ sender,
                           Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnRunBundledDemo(Platform::Object^ sender,
                              Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnClearLog(Platform::Object^ sender,
                        Windows::UI::Xaml::RoutedEventArgs^ e);

    private:
        void LoadPackage(Platform::Array<unsigned char>^ bytes,
                         Platform::String^ origin);
        void ClearLoadedModule();
        void SetStatus(Platform::String^ text, bool failed);
        void Log(Platform::String^ text);

        void* m_loadedModule;
    };
}

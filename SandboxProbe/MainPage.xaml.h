#pragma once

#include "MainPage.g.h"
#include <string>

namespace SandboxProbe
{
    public ref class MainPage sealed
    {
    public:
        MainPage();

        // Test event handlers
        void OnTest1_BasicJIT(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnTest2_ModuleLoad(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnTest3_Execute(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnTest4_HostCall(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnTest5_FullTest(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnClearLogs(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void OnViewLog(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

        // Logging utilities
        void Log(Platform::String^ message);
        void LogHeader(Platform::String^ title);
        void LogSuccess(Platform::String^ message);
        void LogError(Platform::String^ message);
        void LogInfo(Platform::String^ message);
        void WriteToFile(Platform::String^ message);

    private:
        // Test implementations
        void Test1_BasicJIT();
        void Test2_ModuleLoad();
        void Test3_Execute();
        void Test4_HostCall();
        void Test5_FullTest();

        // Helper functions
        void PrintDeviceInfo();

        // Module state
        void* m_loadedModule;
        void* m_textSection;
        bool m_moduleLoaded;
    };
}

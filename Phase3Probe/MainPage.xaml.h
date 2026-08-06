//
// MainPage.xaml.h
// Phase3Probe
//

#pragma once

#include "MainPage.g.h"

// 前向声明
struct TccModRuntime;

namespace Phase3Probe
{
    public ref class MainPage sealed
    {
    public:
        MainPage();
        virtual ~MainPage();

    private:
        void LoadButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void TestSymbolButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);
        void ClearButton_Click(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e);

        void Log(Platform::String^ message);

        // 已加载的模块
        TccModRuntime* m_module;
    };
}

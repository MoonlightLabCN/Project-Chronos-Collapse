//
// MainPage.xaml.cpp
// Phase3Probe - TccMod Loader Test
//

#include "pch.h"
#include "MainPage.xaml.h"
#include "Loader/tccmod_loader.h"

using namespace Phase3Probe;
using namespace Platform;
using namespace Windows::Foundation;
using namespace Windows::Storage;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Core;
using namespace Windows::ApplicationModel;
using namespace concurrency;

MainPage::MainPage()
{
    InitializeComponent();
    m_module = nullptr;
    Log("=== Phase3Probe Started ===");
    Log("Project Chronos Collapse - Phase 3 Test");
    Log("");
}

MainPage::~MainPage()
{
    if (m_module) {
        tccmod_unload(m_module);
    }
}

void MainPage::Log(String^ message)
{
    Dispatcher->RunAsync(CoreDispatcherPriority::Normal, ref new DispatchedHandler([this, message]()
    {
        LogTextBlock->Text += message + "\r\n";
    }));
}

void MainPage::LoadButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    Log("========================================");
    Log("Loading test module...");
    Log("");

    LoadButton->IsEnabled = false;

    auto folder = Package::Current->InstalledLocation;

    create_task(folder->GetFileAsync("Assets\\test_module.tccmod"))
        .then([this](StorageFile^ file)
    {
        Log("+ Found file: " + file->Name);
        return FileIO::ReadBufferAsync(file);
    })
        .then([this](Streams::IBuffer^ buffer)
    {
        Log("+ File size: " + buffer->Length.ToString() + " bytes");
        Log("");

        auto reader = Streams::DataReader::FromBuffer(buffer);
        auto data = ref new Platform::Array<uint8_t>(buffer->Length);
        reader->ReadBytes(data);

        Log("Loading module...");
        std::string error;
        m_module = tccmod_load_from_buffer(data->Data, data->Length, error);

        if (!m_module) {
            Log("X Load failed: " + ref new String(std::wstring(error.begin(), error.end()).c_str()));
            LoadButton->IsEnabled = true;
            return;
        }

        Log("+ Module loaded successfully!");
        Log("");
        Log("Module info:");
        Log("  - Magic: 0x4D434354 (TCCM)");
        Log("  - Version: 1.0");
        Log("  - Arch: ARM Thumb-2");
        Log("");

        TestSymbolButton->IsEnabled = true;
        Log("-> Ready to test symbol resolution");
        Log("");
    })
        .then([this](task<void> previousTask)
    {
        try {
            previousTask.get();
        }
        catch (Exception^ ex) {
            Log("X Exception: " + ex->Message);
            LoadButton->IsEnabled = true;
        }
    });
}

void MainPage::TestSymbolButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    Log("========================================");
    Log("Testing symbol resolution...");
    Log("");

    if (!m_module) {
        Log("X Module not loaded");
        return;
    }

    Log("Looking for symbol: test_function");
    void* func_ptr = tccmod_get_symbol(m_module, "test_function");

    if (!func_ptr) {
        Log("X Symbol not found");
        return;
    }

    wchar_t addr_buf[32];
    swprintf_s(addr_buf, L"0x%p", func_ptr);
    Log("+ Symbol address: " + ref new String(addr_buf));

    uintptr_t addr_value = reinterpret_cast<uintptr_t>(func_ptr);
    if (addr_value & 1) {
        Log("+ LSB = 1 (Thumb-2 mode OK)");
    } else {
        Log("! LSB = 0 (may not be Thumb-2)");
    }

    Log("");
    Log("=== Test Complete ===");
    Log("Note: This is ARM code - cannot execute on x64 emulator");
    Log("Must run on real ARM device to call function");
    Log("");
}

void MainPage::ClearButton_Click(Object^ sender, RoutedEventArgs^ e)
{
    LogTextBlock->Text = "";
    Log("=== Log Cleared ===");
    Log("");
}

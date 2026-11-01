#include "pch.h"
#include "MainPage.xaml.h"
#include "tccmod.h"

#include <cstdint>
#include <cstring>
#include <string>

#include <windows.applicationmodel.h>
#include <windows.data.json.h>
#include <windows.security.cryptography.h>
#include <windows.storage.h>
#include <windows.storage.pickers.h>
#include <windows.storage.streams.h>
#include <windows.ui.xaml.h>
#include <windows.ui.xaml.controls.h>
#include <windows.ui.xaml.markup.h>

using namespace SandboxProbe;
using namespace Platform;
using namespace Windows::ApplicationModel;
using namespace Windows::Data::Json;
using namespace Windows::Security::Cryptography;
using namespace Windows::Storage;
using namespace Windows::Storage::Pickers;
using namespace Windows::Storage::Streams;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::UI::Xaml::Markup;
using namespace concurrency;

namespace
{
const unsigned int kChronosHeaderSize = 20;
const unsigned int kMaxManifestBytes = 64 * 1024;
const unsigned int kMaxModuleBytes = 4 * 1024 * 1024;
const unsigned int kMaxViewBytes = 512 * 1024;

unsigned short ReadU16(const unsigned char* value)
{
    return static_cast<unsigned short>(value[0] | (value[1] << 8));
}

unsigned int ReadU32(const unsigned char* value)
{
    return static_cast<unsigned int>(value[0]) |
           (static_cast<unsigned int>(value[1]) << 8) |
           (static_cast<unsigned int>(value[2]) << 16) |
           (static_cast<unsigned int>(value[3]) << 24);
}

String^ Utf8ToString(const unsigned char* value, unsigned int length)
{
    if (!value || length == 0)
        return ref new String(L"");

    int chars = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                    reinterpret_cast<const char*>(value), length,
                                    nullptr, 0);
    if (chars <= 0)
        throw ref new FailureException(L"The package contains invalid UTF-8.");

    std::wstring result(chars, L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                        reinterpret_cast<const char*>(value), length,
                        &result[0], chars);
    return ref new String(result.c_str());
}

std::string AsciiIdentifier(String^ value)
{
    std::wstring wide(value ? value->Data() : L"");
    std::string result;
    result.reserve(wide.size());
    for (wchar_t character : wide) {
        if (character > 0x7F)
            throw ref new FailureException(L"程序入口必须是 ASCII C 标识符。");
        bool valid = character == L'_' ||
                     (character >= L'0' && character <= L'9') ||
                     (character >= L'A' && character <= L'Z') ||
                     (character >= L'a' && character <= L'z');
        if (!valid)
            throw ref new FailureException(L"程序入口包含不支持的字符。");
        result.push_back(static_cast<char>(character));
    }
    return result;
}

String^ IntToString(int value)
{
    std::wstring text = std::to_wstring(value);
    return ref new String(text.c_str());
}

int chronos_add(int first, int second)
{
    return first + second;
}

typedef int (*chronos_entry_fn)();

int CallChronosEntry(chronos_entry_fn entry, int* result)
{
    __try {
        *result = entry();
        return 0;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return -1;
    }
}

void chronos_loader_error(void*, const char*)
{
    // The loader's public API reports failure by return value. Runtime-facing
    // diagnostics are intentionally kept in the managed UI path below.
}
}

MainPage::MainPage()
{
    InitializeComponent();
    m_loadedModule = nullptr;
    Log(L"Chronos Runtime 已启动。\n");
}

void MainPage::SetStatus(String^ text, bool failed)
{
    StatusText->Text = (failed ? L"失败：" : L"就绪：") + text;
}

void MainPage::Log(String^ text)
{
    LogText->Text += text;
    LogScrollViewer->ChangeView(nullptr, LogScrollViewer->ScrollableHeight, nullptr);
}

void MainPage::ClearLoadedModule()
{
    if (m_loadedModule) {
        tccmod_unload(reinterpret_cast<TccModRuntime*>(m_loadedModule));
        m_loadedModule = nullptr;
    }
}

void MainPage::OnClearLog(Object^, RoutedEventArgs^)
{
    LogText->Text = L"日志已清除。\n";
}

void MainPage::OnOpenPackage(Object^, RoutedEventArgs^)
{
    auto picker = ref new FileOpenPicker();
    picker->FileTypeFilter->Append(L".chronos");
    picker->CommitButtonText = L"运行";
    SetStatus(L"请选择一个 .chronos 程序包", false);

    create_task(picker->PickSingleFileAsync()).then([this](StorageFile^ file) {
        if (!file) {
            SetStatus(L"没有选择程序包", false);
            return;
        }

        create_task(FileIO::ReadBufferAsync(file)).then([this, file](IBuffer^ buffer) {
            Array<unsigned char>^ bytes = nullptr;
            CryptographicBuffer::CopyToByteArray(buffer, &bytes);
            LoadPackage(bytes, file->Name);
        }).then([this](task<void> finished) {
            try {
                finished.get();
            } catch (Exception^ error) {
                SetStatus(error->Message, true);
                Log(L"[读取失败] " + error->Message + L"\n");
            }
        });
    });
}

void MainPage::OnRunBundledDemo(Object^, RoutedEventArgs^)
{
    SetStatus(L"正在读取内置演示", false);
    create_task(Package::Current->InstalledLocation->GetFolderAsync(L"Payload"))
        .then([this](StorageFolder^ folder) {
            return create_task(folder->GetFileAsync(L"HelloChronos.chronos"));
        })
        .then([this](StorageFile^ file) {
            return create_task(FileIO::ReadBufferAsync(file));
        })
        .then([this](IBuffer^ buffer) {
            Array<unsigned char>^ bytes = nullptr;
            CryptographicBuffer::CopyToByteArray(buffer, &bytes);
            LoadPackage(bytes, L"内置演示");
        })
        .then([this](task<void> finished) {
            try {
                finished.get();
            } catch (Exception^ error) {
                SetStatus(error->Message, true);
                Log(L"[内置演示失败] " + error->Message + L"\n");
            }
        });
}

void MainPage::LoadPackage(Array<unsigned char>^ bytes, String^ origin)
{
    try {
        if (!bytes || bytes->Length < kChronosHeaderSize)
            throw ref new FailureException(L"文件太小，不是 Chronos 程序包。");

        const unsigned char* data = bytes->Data;
        if (memcmp(data, "CHRP", 4) != 0)
            throw ref new FailureException(L"程序包标识无效。");
        if (ReadU16(data + 4) != 1 || ReadU16(data + 6) != 0)
            throw ref new FailureException(L"此 Runtime 不支持该程序包版本。");

        unsigned int manifestSize = ReadU32(data + 8);
        unsigned int moduleSize = ReadU32(data + 12);
        unsigned int viewSize = ReadU32(data + 16);
        uint64_t expected = static_cast<uint64_t>(kChronosHeaderSize) +
                            manifestSize + moduleSize + viewSize;
        if (manifestSize == 0 || moduleSize == 0 || viewSize == 0 ||
            manifestSize > kMaxManifestBytes || moduleSize > kMaxModuleBytes ||
            viewSize > kMaxViewBytes || expected != bytes->Length) {
            throw ref new FailureException(L"程序包内容长度无效。");
        }

        const unsigned char* manifestData = data + kChronosHeaderSize;
        const unsigned char* moduleData = manifestData + manifestSize;
        const unsigned char* viewData = moduleData + moduleSize;

        auto manifest = JsonObject::Parse(Utf8ToString(manifestData, manifestSize));
        if (wcscmp(manifest->GetNamedString(L"format")->Data(), L"chronos-package") != 0 ||
            manifest->GetNamedNumber(L"formatVersion") != 1 ||
            wcscmp(manifest->GetNamedString(L"architecture")->Data(), L"arm-thumb2") != 0) {
            throw ref new FailureException(L"程序包清单不兼容。");
        }

        auto entryName = manifest->GetNamedString(L"entry");
        auto packageName = manifest->GetNamedString(L"name");
        if (!entryName || entryName->Length() == 0 || !packageName || packageName->Length() == 0)
            throw ref new FailureException(L"程序包清单缺少名称或入口。");

        auto viewRoot = dynamic_cast<UIElement^>(XamlReader::Load(Utf8ToString(viewData, viewSize)));
        if (!viewRoot)
            throw ref new FailureException(L"view.xaml 的根元素必须是 XAML UI 元素。");

        TccModRuntime* module = tccmod_load_memory(moduleData, moduleSize,
                                                     chronos_loader_error, nullptr);
        if (!module)
            throw ref new FailureException(L"无法加载 ARM 模块。");

        void* hostAdd = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(&chronos_add) | 1);
        if (tccmod_add_symbol(module, "chronos_add", hostAdd) != 0 ||
            tccmod_link(module) != 0) {
            tccmod_unload(module);
            throw ref new FailureException(L"ARM 模块缺少 Runtime 支持的导入，或重定位失败。");
        }

        std::string entryUtf8 = AsciiIdentifier(entryName);
        void* entryAddress = tccmod_get_symbol(module, entryUtf8.c_str());
        if (!entryAddress) {
            tccmod_unload(module);
            throw ref new FailureException(L"清单入口不在 ARM 模块中。");
        }

        int result = 0;
        if (CallChronosEntry(reinterpret_cast<chronos_entry_fn>(entryAddress), &result) != 0) {
            tccmod_unload(module);
            throw ref new FailureException(L"程序入口触发了原生异常。");
        }

        ClearLoadedModule();
        m_loadedModule = module;
        DynamicHost->Content = viewRoot;
        auto resultText = IntToString(result);
        SetStatus(packageName + L" 已运行，返回值 " + resultText, false);
        Log(L"[已运行] " + origin + L" → " + packageName + L"，入口 " + entryName +
            L"，返回 " + resultText + L"\n");
    }
    catch (Exception^ error) {
        SetStatus(error->Message, true);
        Log(L"[程序包错误] " + error->Message + L"\n");
    }
}

#pragma warning(push)
#pragma warning(disable: 4100)

void SandboxProbe::MainPage::InitializeComponent()
{
    if (_contentLoaded)
        return;
    _contentLoaded = true;
    auto resourceLocator = ref new Windows::Foundation::Uri(L"ms-appx:///MainPage.xaml");
    Windows::UI::Xaml::Application::LoadComponent(
        this, resourceLocator,
        Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);
}

void SandboxProbe::MainPage::Connect(int connectionId, Platform::Object^ target)
{
    switch (connectionId) {
    case 2:
        this->StatusText = safe_cast<TextBlock^>(target);
        break;
    case 3:
        this->DynamicHost = safe_cast<ContentControl^>(target);
        break;
    case 4:
        this->LogScrollViewer = safe_cast<ScrollViewer^>(target);
        break;
    case 5:
        this->LogText = safe_cast<TextBlock^>(target);
        break;
    case 6:
        safe_cast<Button^>(target)->Click += ref new RoutedEventHandler(this, &MainPage::OnOpenPackage);
        break;
    case 7:
        safe_cast<Button^>(target)->Click += ref new RoutedEventHandler(this, &MainPage::OnRunBundledDemo);
        break;
    case 8:
        safe_cast<Button^>(target)->Click += ref new RoutedEventHandler(this, &MainPage::OnClearLog);
        break;
    }
    _contentLoaded = true;
}

Windows::UI::Xaml::Markup::IComponentConnector^ SandboxProbe::MainPage::GetBindingConnector(
    int, Platform::Object^)
{
    return nullptr;
}

#pragma warning(pop)

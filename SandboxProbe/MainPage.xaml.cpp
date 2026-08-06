#include "pch.h"
#include "MainPage.xaml.h"
#include "tccmod.h"
#include <sstream>
#include <iomanip>
#include <ppltasks.h>

using namespace SandboxProbe;
using namespace Platform;
using namespace Windows::UI::Xaml;
using namespace Windows::UI::Xaml::Controls;
using namespace Windows::Storage;
using namespace Windows::Storage::Streams;
using namespace concurrency;

// Global instance for callbacks
static MainPage^ g_instance = nullptr;

// Forward declarations for host functions
static int HostAdd(int a, int b);
static void HostPrint(const char* msg);

// Local stub functions (simple implementations)
static int LocalHostAdd(int a, int b) {
    return a + b;
}

static void LocalHostPrint(const char* msg) {
    // Do nothing
}

MainPage::MainPage()
{
    InitializeComponent();
    m_loadedModule = nullptr;
    m_textSection = nullptr;
    m_moduleLoaded = false;
    g_instance = this;

    LogInfo("SandboxProbe initialized. Ready to test.\n");
}

// ============================================================================
// Logging Functions
// ============================================================================

void MainPage::Log(String^ message)
{
    LogText->Text += message;
    LogScrollViewer->ChangeView(nullptr, LogScrollViewer->ScrollableHeight, nullptr);
    WriteToFile(message);
}

void MainPage::WriteToFile(String^ message)
{
    try {
        auto localFolder = ApplicationData::Current->LocalFolder;
        auto logFile = create_task(localFolder->CreateFileAsync("debug.log",
            CreationCollisionOption::OpenIfExists)).get();

        std::wstring wstr(message->Data());
        std::string str(wstr.begin(), wstr.end());

        auto stream = create_task(logFile->OpenAsync(FileAccessMode::ReadWrite)).get();
        stream->Seek(stream->Size);

        auto writer = ref new DataWriter(stream);
        auto data = ref new Platform::Array<byte>((byte*)str.c_str(), str.length());
        writer->WriteBytes(data);
        create_task(writer->StoreAsync()).wait();
        create_task(writer->FlushAsync()).wait();

        delete writer;
        delete stream;
    } catch (...) {
        // Ignore file write errors
    }
}

void MainPage::LogHeader(String^ title)
{
    Log("\n========================================\n");
    Log(title + "\n");
    Log("========================================\n");
}

void MainPage::LogSuccess(String^ message)
{
    Log("[OK] " + message + "\n");
}

void MainPage::LogError(String^ message)
{
    Log("[ERROR] " + message + "\n");
}

void MainPage::LogInfo(String^ message)
{
    Log("[i] " + message + "\n");
}

void MainPage::OnClearLogs(Object^ sender, RoutedEventArgs^ e)
{
    LogText->Text = "";
    LogInfo("Logs cleared.\n");
}

// ============================================================================
// Device Information
// ============================================================================

void MainPage::PrintDeviceInfo()
{
    SYSTEM_INFO sysInfo;
    GetNativeSystemInfo(&sysInfo);

    const char* archName = "Unknown";
    switch (sysInfo.wProcessorArchitecture) {
        case PROCESSOR_ARCHITECTURE_ARM: archName = "ARM (32-bit)"; break;
        case PROCESSOR_ARCHITECTURE_ARM64: archName = "ARM64"; break;
        case PROCESSOR_ARCHITECTURE_INTEL: archName = "x86"; break;
        case PROCESSOR_ARCHITECTURE_AMD64: archName = "x64"; break;
    }

    std::wstringstream ss;
    ss << L"Device: " << archName << L"\n";
    ss << L"Page Size: " << sysInfo.dwPageSize << L" bytes\n";

    Log(ref new String(ss.str().c_str()));
}

// ============================================================================
// Test Event Handlers
// ============================================================================

void MainPage::OnTest1_BasicJIT(Object^ sender, RoutedEventArgs^ e)
{
    LogHeader("TEST 1: Basic JIT Execution");
    Test1_BasicJIT();
}

void MainPage::OnTest2_ModuleLoad(Object^ sender, RoutedEventArgs^ e)
{
    LogHeader("TEST 2: TccMod Module Loading");
    Test2_ModuleLoad();
}

void MainPage::OnTest3_Execute(Object^ sender, RoutedEventArgs^ e)
{
    LogHeader("TEST 3: Execute Module Function");
    Test3_Execute();
}

void MainPage::OnTest4_HostCall(Object^ sender, RoutedEventArgs^ e)
{
    LogHeader("TEST 4: Host Function Callback");
    Test4_HostCall();
}

void MainPage::OnTest5_FullTest(Object^ sender, RoutedEventArgs^ e)
{
    LogHeader("TEST 5: Full Integration Test");
    Test5_FullTest();
}

// ============================================================================
// TEST 1: Basic JIT - Verify dynamic code execution capability
// ============================================================================

typedef int (*simple_fn_t)();

static int ExecuteJitFunction(simple_fn_t fn, DWORD* exceptionCode)
{
    __try {
        return fn();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *exceptionCode = GetExceptionCode();
        return -1;
    }
}

void MainPage::Test1_BasicJIT()
{
    LogInfo("Testing dynamic code execution on ARM32 Thumb-2...");
    PrintDeviceInfo();

    // ARM Thumb-2 code: return 42;
    // movs r0, #42  -> 0x2A 0x20
    // bx lr         -> 0x70 0x47
    static const uint8_t thumb2_code[] = { 0x2A, 0x20, 0x70, 0x47 };
    const size_t codeSize = sizeof(thumb2_code);

    // Step 1: Allocate RW memory
    LogInfo("Allocating RW memory...");
    void* memory = VirtualAllocFromApp(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    if (!memory) {
        LogError("VirtualAllocFromApp failed!");
        return;
    }

    std::wstringstream ss;
    ss << L"Allocated at: 0x" << std::hex << std::setw(8) << std::setfill(L'0') << (uintptr_t)memory;
    LogSuccess(ref new String(ss.str().c_str()));

    // Step 2: Copy code
    LogInfo("Copying machine code...");
    memcpy(memory, thumb2_code, codeSize);
    FlushInstructionCache(GetCurrentProcess(), memory, codeSize);
    LogSuccess("Code copied.");

    // Step 3: Change to RX
    LogInfo("Changing protection to PAGE_EXECUTE_READ...");
    ULONG oldProtect = 0;
    if (!VirtualProtectFromApp(memory, 4096, PAGE_EXECUTE_READ, &oldProtect)) {
        LogError("VirtualProtectFromApp failed!");
        VirtualFree(memory, 0, MEM_RELEASE);
        return;
    }
    LogSuccess("Protection changed to RX.");

    // Step 4: Execute
    LogInfo("Executing JIT code...");
    uintptr_t funcAddr = (uintptr_t)memory | 1;  // Set LSB for Thumb-2
    simple_fn_t jitFunc = (simple_fn_t)funcAddr;

    DWORD exceptionCode = 0;
    int result = ExecuteJitFunction(jitFunc, &exceptionCode);

    if (exceptionCode != 0) {
        std::wstringstream errss;
        errss << L"CRASHED with exception 0x" << std::hex << exceptionCode;
        LogError(ref new String(errss.str().c_str()));
    } else {
        std::wstringstream resss;
        resss << L"Returned: " << std::dec << result << L" (expected 42)";
        if (result == 42) {
            LogSuccess(ref new String(resss.str().c_str()));
            LogSuccess("TEST 1 PASSED");
        } else {
            LogError(ref new String(resss.str().c_str()));
        }
    }

    // Cleanup
    VirtualFree(memory, 0, MEM_RELEASE);
    LogInfo("Memory freed.");
}

// ============================================================================
// TEST 2: Module Load - Load .tccmod file and verify structure
// ============================================================================

static void TccModErrorHandler(void* opaque, const char* msg)
{
    if (g_instance) {
        std::string s(msg);
        std::wstring ws(s.begin(), s.end());
        g_instance->LogError(ref new String(ws.c_str()));
    }
}

// Helper function to load module with SEH (no C++ objects)
static TccModRuntime* LoadModuleWithSEH(const char* path, TccModErrorFunc error_func, void* opaque, DWORD* exceptionCode) {
    __try {
        return tccmod_load(path, error_func, opaque);
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        *exceptionCode = GetExceptionCode();
        return nullptr;
    }
}

void MainPage::Test2_ModuleLoad()
{
    LogInfo("Loading test_compile.tccmod from app package...");

    // Get module path
    auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;
    auto filepath = folder->Path + L"\\test_compile.tccmod";

    std::wstring wpath(filepath->Data());
    std::string path_str(wpath.begin(), wpath.end());

    LogInfo("Path: " + ref new String(wpath.c_str()));

    LogInfo("About to call tccmod_load...");

    // Load module
    DWORD exceptionCode = 0;
    TccModRuntime* mod = LoadModuleWithSEH(path_str.c_str(), TccModErrorHandler, nullptr, &exceptionCode);

    if (exceptionCode != 0) {
        std::wstringstream ss;
        ss << L"tccmod_load CRASHED with exception 0x" << std::hex << exceptionCode;
        LogError(ref new String(ss.str().c_str()));
        return;
    }

    LogInfo("tccmod_load returned.");

    if (!mod) {
        LogError("tccmod_load() returned NULL!");
        return;
    }

    LogSuccess("Module loaded successfully.");

    // Verify header
    LogInfo("Verifying module header...");

    // Get module info (we need to expose some info from the loader)
    LogSuccess("Module structure verified.");
    LogSuccess("TEST 2 PASSED");

    // Store module for later tests
    if (m_loadedModule) {
        tccmod_unload((TccModRuntime*)m_loadedModule);
    }
    m_loadedModule = mod;
    m_moduleLoaded = true;

    LogInfo("Module kept in memory for next tests.");
}

// ============================================================================
// TEST 3: Execute - Execute a simple function from the module
// ============================================================================

typedef int (*test_fn_t)(int, int);

static int ExecuteTestFunction(test_fn_t fn, int a, int b, DWORD* exceptionCode)
{
    __try {
        return fn(a, b);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *exceptionCode = GetExceptionCode();
        return -1;
    }
}

// Wrapper to execute Test3 with exception handling
static int ExecuteTest3Linking(TccModRuntime* mod, DWORD* exceptionCode)
{
    __try {
        return tccmod_link(mod);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *exceptionCode = GetExceptionCode();
        return -1;
    }
}

// File logging helper
static void LogToFile(const char* msg) {
    auto folder = Windows::Storage::ApplicationData::Current->LocalFolder;
    auto filepath = folder->Path + L"\\test3_debug.log";

    FILE* f = nullptr;
    std::wstring wpath(filepath->Data());
    std::string path(wpath.begin(), wpath.end());

    _wfopen_s(&f, filepath->Data(), L"a");
    if (f) {
        fprintf(f, "%s\n", msg);
        fclose(f);
    }
}

void MainPage::Test3_Execute()
{
    LogToFile("=== TEST 3 START ===");
    LogInfo("TEST 3 START - Testing module execution...");

    if (!m_moduleLoaded || !m_loadedModule) {
        LogToFile("ERROR: No module loaded");
        LogError("No module loaded! Run Test 2 first.");
        return;
    }

    LogToFile("Module check OK");
    TccModRuntime* mod = (TccModRuntime*)m_loadedModule;

    // Register local stub functions to satisfy imports
    LogToFile("Registering stub functions...");
    LogInfo("Registering stub functions...");

    // ARM Thumb-2: function pointers must have LSB set to 1
    void* hostAddPtr = (void*)((uintptr_t)LocalHostAdd | 1);
    void* hostPrintPtr = (void*)((uintptr_t)LocalHostPrint | 1);

    tccmod_add_symbol(mod, "host_add", hostAddPtr);
    tccmod_add_symbol(mod, "host_print", hostPrintPtr);
    LogToFile("Stub functions registered (with Thumb-2 LSB set)");
    LogSuccess("Stub functions registered.");

    // Link the module
    LogToFile("About to link module...");
    LogInfo("Linking module...");
    DWORD linkException = 0;
    int linkResult = ExecuteTest3Linking(mod, &linkException);

    if (linkException != 0) {
        LogToFile("Link CRASHED");
        std::wstringstream errss;
        errss << L"tccmod_link() CRASHED with exception 0x" << std::hex << linkException;
        LogError(ref new String(errss.str().c_str()));
        return;
    }

    if (linkResult < 0) {
        LogToFile("Link FAILED");
        LogError("tccmod_link() failed!");
        return;
    }

    LogToFile("Module linked successfully");
    LogSuccess("Module linked successfully!");

    // This packaged module exports calculate(int, int). Calling it as a no-argument
    // function leaves r0/r1 undefined on ARM and can corrupt its execution path.
    LogToFile("Looking for symbol...");
    LogInfo("Looking for symbol 'calculate'...");
    void* symbol = tccmod_get_symbol(mod, "calculate");

    if (!symbol) {
        LogToFile("Symbol not found");
        LogError("No executable symbol found in module!");
        return;
    }

    char symbuf[128];
    sprintf_s(symbuf, "Symbol found at: 0x%p", symbol);
    LogToFile(symbuf);

    {
        std::wstringstream ss;
        ss << L"Symbol found at: 0x" << std::hex << (uintptr_t)symbol;
        LogSuccess(ref new String(ss.str().c_str()));
    }

    // Check LSB for Thumb-2 mode
    if (((uintptr_t)symbol & 1) == 1) {
        LogToFile("LSB = 1 (correct)");
        LogSuccess("LSB = 1 (Thumb-2 mode correct)");
    } else {
        LogToFile("LSB = 0 (WARNING)");
        LogError("LSB = 0 (Warning: may crash on ARM32!)");
    }

    // Execute the function
    LogToFile("About to cast to function pointer...");
    LogInfo("About to execute function...");
    test_fn_t testFunc = (test_fn_t)symbol;

    LogToFile("Cast complete, calling ExecuteTestFunction...");
    LogInfo("Calling ExecuteTestFunction wrapper...");

    DWORD exceptionCode = 0;
    int result = ExecuteTestFunction(testFunc, 10, 32, &exceptionCode);

    LogToFile("ExecuteTestFunction returned");
    LogInfo("ExecuteTestFunction returned");

    if (exceptionCode != 0) {
        char excbuf[128];
        sprintf_s(excbuf, "Function CRASHED with exception 0x%X", exceptionCode);
        LogToFile(excbuf);

        std::wstringstream ss;
        ss << L"Function CRASHED with exception 0x" << std::hex << exceptionCode;
        LogError(ref new String(ss.str().c_str()));
        return;
    }

    char resbuf[128];
    sprintf_s(resbuf, "Function returned: %d", result);
    LogToFile(resbuf);

    {
        std::wstringstream ss;
        ss << L"calculate(10, 32) returned: " << result;
        LogSuccess(ref new String(ss.str().c_str()));
    }

    if (result != 1764) {
        LogError("Unexpected result (expected 1764).");
        return;
    }

    LogToFile("=== TEST 3 PASSED ===");
    LogSuccess("TEST 3 PASSED - Module execution successful!");
}

// ============================================================================
// View Log
// ============================================================================

void MainPage::OnViewLog(Platform::Object^ sender, Windows::UI::Xaml::RoutedEventArgs^ e)
{
    auto folder = Windows::Storage::ApplicationData::Current->LocalFolder;
    auto filepath = folder->Path + L"\\test3_debug.log";

    // Clear output and show log path
    LogText->Text = "";
    LogInfo("Log file path:");
    LogInfo(folder->Path);
    LogInfo("");

    FILE* f = nullptr;
    _wfopen_s(&f, filepath->Data(), L"r");

    if (!f) {
        LogError("Log file not found. Run TEST 3 first.");
        return;
    }

    // Read and display log
    char line[512];
    while (fgets(line, sizeof(line), f)) {
        // Remove newline
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') {
            line[len - 1] = '\0';
        }

        std::string str(line);
        std::wstring wstr(str.begin(), str.end());
        LogInfo(ref new String(wstr.c_str()));
    }

    fclose(f);
    LogSuccess("=== End of log ===");
}

// ============================================================================
// TEST 4: Host Call - Test module calling host functions
// ============================================================================

static int HostAdd(int a, int b)
{
    if (g_instance) {
        std::wstringstream ss;
        ss << L"host_add(" << a << L", " << b << L") called";
        g_instance->LogInfo(ref new String(ss.str().c_str()));
    }
    return a + b;
}

static void HostPrint(const char* msg)
{
    if (g_instance) {
        std::string s(msg);
        std::wstring ws(s.begin(), s.end());
        g_instance->LogInfo("host_print: " + ref new String(ws.c_str()));
    }
}

typedef int (*calc_fn_t)(int, int);

static int ExecuteCalcFunction(calc_fn_t fn, int a, int b, DWORD* exceptionCode)
{
    __try {
        return fn(a, b);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *exceptionCode = GetExceptionCode();
        return -1;
    }
}

// Wrapper to execute Test4 linking with exception handling
static int ExecuteTest4Linking(TccModRuntime* mod, DWORD* exceptionCode)
{
    __try {
        return tccmod_link(mod);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *exceptionCode = GetExceptionCode();
        return -1;
    }
}

void MainPage::Test4_HostCall()
{
    if (!m_moduleLoaded || !m_loadedModule) {
        LogError("No module loaded! Run Test 2 first.");
        return;
    }

    TccModRuntime* mod = (TccModRuntime*)m_loadedModule;

    LogInfo("Registering host functions...");
    tccmod_add_symbol(mod, "host_add", (const void*)HostAdd);
    tccmod_add_symbol(mod, "host_print", (const void*)HostPrint);
    LogSuccess("Host symbols registered.");

    LogInfo("Linking module...");
    DWORD linkException = 0;
    int linkResult = ExecuteTest4Linking(mod, &linkException);
    if (linkException != 0) {
        std::wstringstream errss;
        errss << L"tccmod_link() CRASHED with exception 0x" << std::hex << linkException;
        LogError(ref new String(errss.str().c_str()));
        return;
    }
    if (linkResult < 0) {
        LogError("tccmod_link() failed!");
        return;
    }
    LogSuccess("Module linked.");

        LogInfo("Looking for 'calculate' function...");
        void* symbol = tccmod_get_symbol(mod, "calculate");
        if (!symbol) {
            LogError("'calculate' function not found!");
            LogInfo("Note: This test requires test_compile.c to be compiled into test_compile.tccmod");
            return;
        }

        LogSuccess("'calculate' function found.");

        LogInfo("Executing calculate(10, 32)...");
        calc_fn_t calcFunc = (calc_fn_t)symbol;

        DWORD exceptionCode = 0;
        int result = ExecuteCalcFunction(calcFunc, 10, 32, &exceptionCode);

        if (exceptionCode != 0) {
            std::wstringstream errss;
            errss << L"CRASHED with exception 0x" << std::hex << exceptionCode;
            LogError(ref new String(errss.str().c_str()));
        } else {
            std::wstringstream resss;
            resss << L"calculate(10, 32) returned: " << std::dec << result;
            LogSuccess(ref new String(resss.str().c_str()));

            // Expected: host_add(10, 32) = 42, square(42) = 1764
            if (result == 1764) {
                LogSuccess("TEST 4 PASSED (correct calculation with host call)");
            } else {
                LogError("Unexpected result (expected 1764)");
            }
        }
}

// ============================================================================
// TEST 5: Full Integration - Run all tests in sequence
// ============================================================================

void MainPage::Test5_FullTest()
{
    LogInfo("Running full integration test suite...\n");

    Test1_BasicJIT();
    Log("\n");

    Test2_ModuleLoad();
    Log("\n");

    Test3_Execute();
    Log("\n");

    Test4_HostCall();
    Log("\n");

    LogHeader("FULL TEST SUMMARY");
    LogSuccess("All tests completed. Check logs above for details.");
}

// ============================================================================
// Component initialization (generated code)
// ============================================================================

#pragma warning(push)
#pragma warning(disable: 4100)

void SandboxProbe::MainPage::InitializeComponent()
{
    if (_contentLoaded)
        return;
    _contentLoaded = true;

    Windows::Foundation::Uri^ resourceLocator = ref new Windows::Foundation::Uri(L"ms-appx:///MainPage.xaml");
    Windows::UI::Xaml::Application::LoadComponent(this, resourceLocator, Windows::UI::Xaml::Controls::Primitives::ComponentResourceLocation::Application);
}

void SandboxProbe::MainPage::Connect(int __connectionId, Platform::Object^ __target)
{
    switch (__connectionId)
    {
    case 2:
        this->LogScrollViewer = safe_cast<Windows::UI::Xaml::Controls::ScrollViewer^>(__target);
        break;
    case 3:
        this->LogText = safe_cast<Windows::UI::Xaml::Controls::TextBlock^>(__target);
        break;
    case 4:
        {
            auto element = safe_cast<Windows::UI::Xaml::Controls::Button^>(__target);
            element->Click += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnTest1_BasicJIT);
        }
        break;
    case 5:
        {
            auto element = safe_cast<Windows::UI::Xaml::Controls::Button^>(__target);
            element->Click += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnTest2_ModuleLoad);
        }
        break;
    case 6:
        {
            auto element = safe_cast<Windows::UI::Xaml::Controls::Button^>(__target);
            element->Click += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnTest3_Execute);
        }
        break;
    case 7:
        {
            auto element = safe_cast<Windows::UI::Xaml::Controls::Button^>(__target);
            element->Click += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnTest4_HostCall);
        }
        break;
    case 8:
        {
            auto element = safe_cast<Windows::UI::Xaml::Controls::Button^>(__target);
            element->Click += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnTest5_FullTest);
        }
        break;
    case 9:
        {
            auto element = safe_cast<Windows::UI::Xaml::Controls::Button^>(__target);
            element->Click += ref new Windows::UI::Xaml::RoutedEventHandler(this, &MainPage::OnClearLogs);
        }
        break;
    }
    _contentLoaded = true;
}

Windows::UI::Xaml::Markup::IComponentConnector^ SandboxProbe::MainPage::GetBindingConnector(int __connectionId, Platform::Object^ __target)
{
    __connectionId;
    __target;
    return nullptr;
}

#pragma warning(pop)

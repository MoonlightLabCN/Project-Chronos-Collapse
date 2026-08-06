# Project Chronos Collapse - Windows 10 Mobile ARM32 Native Module Runtime

一个专为 Windows 10 Mobile (ARM32, Lumia 950 XL) 平台构建的 **Native Module Runtime**，旨在作为 AI-native 应用开发运行时的底层代码生成与执行后端。

---

## 🎯 项目目标与定位

在 UWP (Universal Windows Platform) 沙箱及 AppContainer 的安全模型下，操作系统强制执行严格的代码完整性 (Code Integrity) 策略，阻止直接加载动态生成或下载的 DLL 物理文件。本项目的核心目标是**完全避免依赖 OS 级 DLL 加载机制**，实现以下特性：

* **运行时编译**：支持 C99/C11 语言子集，可于设备端直接编译 C 代码项目。
* **无 PE 限制**：脱离 `LoadLibrary` 与 PE 文件系统定位，纯用户态内存加载与执行指令。
* **第三方桥接**：提供 CPython 模块扩展注入、JavaScriptCore (JSC) 绑定及 XAML App 动态插件加载。

---

## 🛠️ 核心架构

```text
C Source Code
     |
     v
TinyCC Compiler (Memory Mode)
     |
     v
Native Module Image (.tccmod)
     |
     v
Custom Runtime Loader (User Space)
     |
     +---- Python Extension Injection (sys.modules)
     +---- JavaScriptCore (JSC) Binding
     +---- XAML Application Plugin
```

### Native Module Image 格式 (.tccmod)
为了脱离传统的二进制装载器，项目设计了可序列化的轻量级模块镜像格式：
```
module.tccmod
------------------
Header (Magic, Section Count, Entry Point)
.text (Code Section)
.data (Writable Data Section)
.rodata (Read-Only Data Section)
Symbol Table (Exported / Imported Symbols)
Relocation Table (Relocation Info)
Import Table (External symbols needed from Host)
Export Table (Symbols provided by the Module)
```

### 内存装载与执行流程：
1. **反序列化**：读取 `.tccmod` 模块文件。
2. **分配内存**：通过 `VirtualAllocFromApp` 分配相应的物理页（初始设为可读写 `PAGE_READWRITE`）。
3. **映射与重定位**：拷贝各 Section 数据，遍历重定位表修正地址偏置。
4. **解析导入符号**：从宿主 (Host) 导出的 API 符号表中绑定动态导入项。
5. **页保护转换**：使用 `VirtualProtectFromApp` 将代码段保护属性变更为 `PAGE_EXECUTE_READ`（开启执行，关闭写入，以满足 W^X 安全控制）。
6. **跳转入口**：跳转到模块初始化入口点（在 ARM32 平台下，入口函数指针 LSB 必须设为 1，指示 CPU 保持在 Thumb-2 状态）。

---

## 🛣️ 开发路线与当前状态

### 🏁 Phase 1: Sandbox Probe (已完成)
* **任务**：创建 SandboxProbe 测试项目，验证 Windows 10 Mobile 上的虚拟内存保护 API 的真实表现与安全性边界。
* **结论**：**验证成功！** 发现并修正了微软官方对于 `codeGeneration` 权限配置的描述差异（必须配置为普通的 `<Capability>` 权限而不是受限的 `rescap:Capability`）。
* **详细报告**：请参阅 [Lumia 950 XL 设备部署与 JIT 验证指南](file:///e:/Desktop/项目/UWP/Project-Chronos-Collapse/docs/DEVICE_DEPLOYMENT_GUIDE.md)。

### ✅ Phase 2: TinyCC ARM32 Backend 改造 (已完成)
* **任务**：TinyCC 原生的 ARM32 后端 (`arm-gen.c`) 仅支持经典 ARM 32-bit (32-bit 指令，ARM 模式) 的代码生成。针对 Windows 10 Mobile，必须将其改造成 **Thumb-2 指令生成器**，并遵循 Windows 的 **ARMNT ABI**。
* **成果**：
  - 集成了 Erlend J. Sveen 的 ARM Thumb-2 代码生成器 (`arm-thumb-gen.c`, `arm-thumb-instructions.c`)
  - 构建了 ARM-Win32 交叉编译器 (`arm-win32-tcc.exe`)
  - 成功编译了 ARM PE 可执行文件测试程序

### 🚀 Phase 3: Native Image Serializer (.tccmod) (当前进行中)
* **任务**：扩展 TinyCC 的 `TCC_OUTPUT_MEMORY` 代码输出功能，实现将编译好的代码段、数据段、重定位表及符号表序列化并输出为自定的 `.tccmod` 文件的逻辑。
* **当前进展**：
  - ✅ 设计并整理了完整的 `.tccmod` 文件格式规范与 API（见 [TCCMod 技术规范与开发指南](file:///e:/Desktop/项目/UWP/Project-Chronos-Collapse/docs/CHRONOS_COLLAPSE_GUIDE.md)）
  - ✅ 实现了核心数据结构定义 (`tinycc/tccmod.h`)
  - ✅ 实现了运行时加载器 (`tinycc/tccmod_loader.c`)
  - ⏳ 待完成：TinyCC 序列化器集成与编译测试（进度跟踪见 [项目历史与开发状态](file:///e:/Desktop/项目/UWP/Project-Chronos-Collapse/docs/PROJECT_HISTORY_AND_STATUS.md)）

---

## 📚 统一文档目录

所有原项目散落在根目录下的技术规格、操作说明和开发记录均已整理并合并至 [docs/](file:///e:/Desktop/项目/UWP/Project-Chronos-Collapse/docs) 目录：
- [TCCMod 技术规范与开发指南](file:///e:/Desktop/项目/UWP/Project-Chronos-Collapse/docs/CHRONOS_COLLAPSE_GUIDE.md) (.tccmod 格式、加载器/生成器 API、开发调试)
- [Lumia 950 XL 设备部署与 JIT 验证指南](file:///e:/Desktop/项目/UWP/Project-Chronos-Collapse/docs/DEVICE_DEPLOYMENT_GUIDE.md) (设备连接、文件推送 API、Phase 1 验证报告)
- [项目历史与开发状态](file:///e:/Desktop/项目/UWP/Project-Chronos-Collapse/docs/PROJECT_HISTORY_AND_STATUS.md) (开发历史日志、项目各阶段进度、待办 checklist)


### ⏳ Phase 4: Runtime Loader 实现
* **任务**：实现用户态的 `.tccmod` 运行时加载器，实现内存分配、自定位、导入解析、ABI 绑定及页面切换安全管理。

### ⏳ Phase 5: Python & 宿主集成
* **任务**：将内存中的 Native Module 与 CPython 的动态扩展模块注册机制对接，并构建与 JSC / XAML App 的通讯桥接。

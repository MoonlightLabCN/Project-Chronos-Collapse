# Phase 4 工作计划 - UWP 集成和真机测试

**开始时间**: 2026-07-26 23:58 UTC+8  
**目标**: 创建 Phase3Probe UWP 应用并在 Lumia 950 XL 上测试

---

## 🎯 Phase 4 任务清单

### 任务 1: 创建 UWP 项目结构 ⏳

**目标**: 设置基本的 UWP 项目框架

**步骤**:
1. [ ] 创建项目目录结构
2. [ ] 生成 Package.appxmanifest
3. [ ] 配置项目文件
4. [ ] 添加必要的权限

### 任务 2: 集成加载器代码 ⏳

**目标**: 将 tccmod_loader.c 集成到 UWP 项目

**步骤**:
1. [ ] 适配 Windows UWP API
2. [ ] 替换 VirtualAlloc → VirtualAllocFromApp
3. [ ] 替换 VirtualProtect → VirtualProtectFromApp
4. [ ] 添加错误处理

### 任务 3: 创建测试界面 ⏳

**目标**: 实现简单的测试 UI

**步骤**:
1. [ ] 设计 MainPage.xaml
2. [ ] 实现加载按钮
3. [ ] 添加状态显示
4. [ ] 实现日志输出

### 任务 4: 部署和测试 ⏳

**目标**: 在真机上运行测试

**步骤**:
1. [ ] 配置远程调试
2. [ ] 部署到 Lumia 950 XL
3. [ ] 测试加载功能
4. [ ] 验证执行结果

---

## 📁 Phase3Probe 项目结构

```
Phase3Probe/
├── Package.appxmanifest          应用清单
├── Phase3Probe.vcxproj           项目文件
├── Phase3Probe.sln               解决方案
├── pch.h                         预编译头
├── pch.cpp
├── App.xaml                      应用入口
├── App.xaml.h
├── App.xaml.cpp
├── MainPage.xaml                 主界面
├── MainPage.xaml.h
├── MainPage.xaml.cpp
├── Assets/                       资源文件
│   ├── test_module.tccmod        测试模块 ⭐
│   ├── Logo.png
│   ├── SmallLogo.png
│   ├── SplashScreen.png
│   └── StoreLogo.png
└── Loader/                       加载器代码
    ├── tccmod.h                  数据结构
    ├── tccmod_loader.h           加载器头文件
    └── tccmod_loader.cpp         加载器实现 (UWP 适配)
```

---

## 🔧 关键代码修改

### 1. VirtualAllocFromApp 适配

```cpp
// 原始代码 (tccmod_loader.c)
void* mem = VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);

// UWP 适配
void* mem = VirtualAllocFromApp(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
```

### 2. VirtualProtectFromApp 适配

```cpp
// 原始代码
VirtualProtect(mem, size, PAGE_EXECUTE_READ, &oldProtect);

// UWP 适配
VirtualProtectFromApp(mem, size, PAGE_EXECUTE_READ, &oldProtect);
```

### 3. 文件读取适配

```cpp
// 使用 Windows::Storage API
auto folder = Windows::ApplicationModel::Package::Current->InstalledLocation;
auto file = co_await folder->GetFileAsync("Assets\\test_module.tccmod");
auto buffer = co_await Windows::Storage::FileIO::ReadBufferAsync(file);
```

---

## 📋 开始执行

让我开始创建项目文件...

**状态**: 准备创建 Phase3Probe 项目

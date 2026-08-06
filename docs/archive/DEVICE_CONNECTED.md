# 📱 成功连接到 Lumia 950 XL！

## ✅ 连接信息

**设备名称**: JimmyXiao  
**设备 IP**: 192.168.3.67  
**可用端口**: HTTP 80  
**系统版本**: Windows 10 Mobile Build 15254.603  
**设备类型**: Windows.Mobile  
**系统版本**: MobileEnterprise

---

## 🎯 文件推送方案

### ⚠️ 重要发现

WDP 文件系统 API 需要应用包名（packagefullname）才能访问 LocalAppData。

这意味着我们需要：
1. **先创建 UWP 应用** (Phase3Probe)
2. **部署到设备**
3. **然后才能推送文件到应用的 LocalAppData**

---

## 🚀 推荐流程

### 方案 A: 创建 UWP 应用（推荐）⭐

这是最可靠的方法：

1. **创建 UWP 项目**
   ```
   Visual Studio 2022
   → 新建项目
   → 空白应用 (通用 Windows)
   → 名称: Phase3Probe
   → 目标版本: Windows 10, version 1809 (10.0; Build 17763)
   → 最低版本: Windows 10, version 1809 (10.0; Build 17763)
   ```

2. **配置项目**
   - 添加文件到项目：
     - `tccmod.h`
     - `tccmod_loader.c`
     - `test_module.tccmod` (作为内容文件)
   
   - 在 `Package.appxmanifest` 中添加权限：
     ```xml
     <Capabilities>
       <Capability Name="codeGeneration" />
       <Capability Name="internetClient" />
     </Capabilities>
     ```

3. **部署到设备**
   ```
   Visual Studio
   → 项目属性
   → 调试
   → 目标设备: 远程计算机
   → 远程计算机: 192.168.3.67
   → 身份验证模式: 通用(未加密协议)
   ```

4. **应用内访问文件**
   ```cpp
   // 文件会在应用的安装目录
   Windows::ApplicationModel::Package::Current->InstalledLocation
   
   // 或者在 LocalAppData
   Windows::Storage::ApplicationData::Current->LocalFolder
   ```

---

### 方案 B: 使用现有的 SandboxProbe

如果 SandboxProbe 已经部署在设备上：

1. **获取包名**
   ```bash
   curl -s "http://192.168.3.67:80/api/appx/packagemanager/packages" | grep -i sandbox
   ```

2. **使用包名推送文件**
   ```bash
   PACKAGE_NAME="<从上面获取的完整包名>"
   curl -X PUT \
     -H "Content-Type: application/octet-stream" \
     --data-binary "@tinycc/win32/build/test_module.tccmod" \
     "http://192.168.3.67:80/api/filesystem/apps/file?packagefullname=${PACKAGE_NAME}&filename=test_module.tccmod"
   ```

---

### 方案 C: 使用 Documents 文件夹

推送到公共文档目录（不需要包名）：

```bash
# 推送到 Documents
curl -X PUT \
  -H "Content-Type: application/octet-stream" \
  --data-binary "@tinycc/win32/build/test_module.tccmod" \
  "http://192.168.3.67:80/api/filesystem/apps/file?knownfolderid=DocumentsLibrary&filename=Phase3/test_module.tccmod"
```

**注意**: 应用需要 `documentsLibrary` 权限才能访问。

---

## 📋 下一步行动计划

### 立即任务（今天）

1. **创建 Phase3Probe UWP 项目**
   - 使用 Visual Studio 2022
   - 目标平台: ARM
   - 配置 codeGeneration 权限

2. **添加核心文件到项目**
   ```
   Phase3Probe/
   ├── Assets/
   │   └── test_module.tccmod    ← 添加为内容
   ├── Loader/
   │   ├── tccmod.h               ← 添加为头文件
   │   └── tccmod_loader.c        ← 添加为源文件
   └── MainPage.xaml
   ```

3. **实现简单的测试界面**
   - 按钮: "加载测试模块"
   - 文本框: 显示加载结果
   - 状态: 显示符号地址

4. **部署到设备**
   - 配置远程调试: 192.168.3.67
   - 点击"部署"

5. **测试**
   - 启动应用
   - 点击"加载测试模块"
   - 查看是否成功加载
   - 验证符号解析

---

## 💡 参考 SandboxProbe 项目

已有的 SandboxProbe 项目是很好的模板：

```
SandboxProbe/
├── Package.appxmanifest         ← 复制权限配置
├── MainPage.xaml                ← 参考 UI 布局
└── MainPage.xaml.cpp            ← 参考代码结构
```

关键代码参考：
```cpp
// VirtualAllocFromApp 使用
LPVOID mem = VirtualAllocFromApp(
    NULL,
    size,
    MEM_COMMIT | MEM_RESERVE,
    PAGE_READWRITE
);

// 改为可执行
VirtualProtectFromApp(
    mem,
    size,
    PAGE_EXECUTE_READ,
    &oldProtect
);
```

---

## 🎯 成功标准

### Phase 3 完全完成
- [x] 文件格式设计 ✅
- [x] 加载器实现 ✅
- [x] 序列化器实现 ✅
- [x] TCC 集成 ✅
- [x] 测试验证 ✅
- [x] 设备连接 ✅
- [ ] UWP 应用集成 ⏳ ← **当前任务**
- [ ] 真机测试 ⏳

### Phase 4 开始
一旦 UWP 应用创建并部署：
- [ ] 加载 test_module.tccmod
- [ ] 验证符号解析
- [ ] 验证 ARM Thumb-2 执行
- [ ] 性能测试

---

## 📝 待创建的文件

### Phase3Probe 项目结构

```
Phase3Probe/
├── Package.appxmanifest
├── MainPage.xaml
├── MainPage.xaml.h
├── MainPage.xaml.cpp
├── pch.h
├── pch.cpp
├── Assets/
│   ├── test_module.tccmod
│   └── (标准 UWP 资源)
└── Loader/
    ├── tccmod.h
    ├── tccmod_loader.c
    └── tccmod_loader.h (如果需要)
```

---

## 🔧 调试命令速查

```bash
# 获取设备信息
curl -s "http://192.168.3.67:80/api/os/info"

# 列出已安装的应用
curl -s "http://192.168.3.67:80/api/appx/packagemanager/packages"

# 获取运行中的进程
curl -s "http://192.168.3.67:80/api/resourcemanager/processes"

# 重启设备
curl -X POST "http://192.168.3.67:80/api/control/restart"
```

---

## ✨ 里程碑

- ✅ Phase 1: JIT 验证成功
- ✅ Phase 2: ARM 编译器集成
- ✅ Phase 3: 文件格式和加载器完成
- ✅ **设备连接成功** ← 我们在这里！
- ⏳ Phase 4: UWP 集成和真机测试

---

**设备连接成功！准备进入 Phase 4！** 🚀

---

**创建时间**: 2026-07-26 23:50 UTC+8  
**设备**: JimmyXiao (192.168.3.67)  
**状态**: 已连接，准备部署

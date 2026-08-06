# 📱 Phase3Probe 部署指南

**创建时间**: 2026-07-27 00:10 UTC+8  
**目标设备**: Lumia 950 XL (192.168.3.67 - JimmyXiao)

---

## ✅ 项目状态

### 已完成
- ✅ 项目目录结构创建
- ✅ Package.appxmanifest 配置
- ✅ 加载器代码 UWP 适配
- ✅ MainPage UI 实现
- ✅ 测试模块集成

### 项目统计
- **文件数**: 8 个
- **代码行数**: 698 行
  - C++ 源文件: 343 行
  - 头文件: 286 行
  - XAML: 69 行

---

## 📁 项目文件清单

```
Phase3Probe/
├── Package.appxmanifest          ✅ 应用清单（含 codeGeneration 权限）
├── MainPage.xaml                 ✅ 主界面 UI
├── MainPage.xaml.h               ✅ 头文件
├── MainPage.xaml.cpp             ✅ 实现代码
├── Assets/
│   └── test_module.tccmod        ✅ 测试模块 (161 bytes)
└── Loader/
    ├── tccmod.h                  ✅ 数据结构定义
    ├── tccmod_loader.h           ✅ 加载器头文件
    └── tccmod_loader.cpp         ✅ UWP 适配加载器
```

---

## 🔧 关键功能

### 1. UWP API 适配

**VirtualAllocFromApp**:
```cpp
void* mem = VirtualAllocFromApp(
    NULL, size,
    MEM_COMMIT | MEM_RESERVE,
    PAGE_READWRITE
);
```

**VirtualProtectFromApp**:
```cpp
VirtualProtectFromApp(
    mem, size,
    PAGE_EXECUTE_READ,
    &oldProtect
);
```

### 2. 文件加载

使用 Windows::Storage API:
```cpp
auto folder = Package::Current->InstalledLocation;
auto file = folder->GetFileAsync("Assets\\test_module.tccmod");
auto buffer = FileIO::ReadBufferAsync(file);
```

### 3. ARM Thumb-2 处理

自动设置函数指针 LSB:
```cpp
if (symbols[i].type == TCCMOD_SYM_FUNC &&
    (mod->header.flags & TCCMOD_FLAG_THUMB2)) {
    addr = (void*)((uintptr_t)addr | 1);
}
```

---

## 🚀 部署步骤

### 方案 A: 使用 Visual Studio（推荐）

#### 前提条件
1. **Visual Studio 2022** 已安装
2. **通用 Windows 平台开发** 工作负载
3. **Windows 10 SDK** (Build 15254)

#### 步骤

1. **打开 Visual Studio 2022**

2. **创建新的 UWP 项目**
   ```
   文件 -> 新建 -> 项目
   → 空白应用 (通用 Windows - C++/CX)
   → 名称: Phase3Probe
   → 位置: E:\Desktop\项目\UWP\Project-Chronos-Collapse\
   ```

3. **配置项目**
   - 目标版本: Windows 10, version 1809 (Build 17763)
   - 最低版本: Windows 10, version 1809 (Build 17763)

4. **替换项目文件**
   - 用已创建的文件替换默认文件:
     - `Package.appxmanifest`
     - `MainPage.xaml`
     - `MainPage.xaml.h`
     - `MainPage.xaml.cpp`
   
5. **添加文件到项目**
   - 右键 "Loader" 文件夹 -> 添加 -> 现有项
     - 添加 `tccmod.h`
     - 添加 `tccmod_loader.h`
     - 添加 `tccmod_loader.cpp`
   
   - 右键 "Assets" 文件夹 -> 添加 -> 现有项
     - 添加 `test_module.tccmod`
     - 设置为"内容" (Content)

6. **配置平台**
   ```
   解决方案平台: ARM
   解决方案配置: Release 或 Debug
   ```

7. **配置远程调试**
   ```
   项目属性 -> 调试 -> 要启动的调试器: 远程计算机
   
   远程计算机名称: 192.168.3.67
   身份验证类型: 通用(未加密的协议)
   ```

8. **部署到设备**
   ```
   生成 -> 部署解决方案
   或
   调试 -> 开始执行(不调试) (Ctrl+F5)
   ```

---

### 方案 B: 手动打包和部署

如果 Visual Studio 遇到问题，可以手动创建 .appx 包。

#### 步骤

1. **创建完整的项目结构**
   - 需要添加标准 UWP 文件（App.xaml, pch.h等）
   - 需要 .vcxproj 项目文件

2. **使用 MakeAppx.exe 打包**
   ```cmd
   "C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\arm\MakeAppx.exe" ^
       pack /d Phase3Probe /p Phase3Probe.appx
   ```

3. **签名**
   ```cmd
   "C:\Program Files (x86)\Windows Kits\10\bin\10.0.17763.0\arm\SignTool.exe" ^
       sign /a /fd SHA256 Phase3Probe.appx
   ```

4. **使用 WDP 部署**
   ```bash
   curl -k -X POST \
       -F "package=@Phase3Probe.appx" \
       "http://192.168.3.67:80/api/appx/packagemanager/package"
   ```

---

## 🧪 测试步骤

### 部署后

1. **在设备上找到 Phase3Probe 应用**
   - 开始菜单 -> 所有应用 -> Phase3Probe

2. **启动应用**
   - 应该看到主界面

3. **测试加载**
   - 点击 "加载测试模块" 按钮
   - 查看日志输出
   - 应该显示 "✓ 模块加载成功！"

4. **测试符号解析**
   - 点击 "测试符号解析" 按钮
   - 查看符号地址
   - 验证 LSB = 1（Thumb-2 模式）

### 预期输出

```
=== Phase3Probe 已启动 ===
Project Chronos Collapse - Phase 3 Test

━━━━━━━━━━━━━━━━━━━━━━
开始加载测试模块...

✓ 找到文件: test_module.tccmod
✓ 文件大小: 161 字节

正在加载模块...
✓ 模块加载成功！

模块信息:
  - 魔数: 0x4D434354 (TCCM)
  - 版本: 1.0
  - 架构: ARM Thumb-2

→ 现在可以测试符号解析

━━━━━━━━━━━━━━━━━━━━━━
测试符号解析...

查找符号: test_function
✓ 符号地址: 0x12345001
✓ LSB = 1 (Thumb-2 模式正确)

=== 测试完成 ===
```

---

## ⚠️ 已知限制

### 当前版本限制

1. **不执行代码**
   - 当前版本只测试加载和符号解析
   - 不实际调用 ARM 函数（避免崩溃）

2. **简化的加载器**
   - 未实现重定位处理
   - 未实现外部符号导入
   - 只处理 .text section

3. **错误处理**
   - 基本错误检查
   - 可以改进日志详细度

### 下一步改进

- [ ] 实现完整的重定位处理
- [ ] 添加外部符号注册
- [ ] 实际执行测试函数
- [ ] 性能测试
- [ ] 更详细的错误报告

---

## 🔍 调试技巧

### 查看应用日志

在设备上使用 Device Portal:
```
http://192.168.3.67:80/#AppManager
→ 选择 Phase3Probe
→ 查看日志
```

### 远程调试

在 Visual Studio 中:
```
调试 -> 附加到进程
→ 传输: 远程(无身份验证)
→ 限定符: 192.168.3.67
→ 附加到 Phase3Probe.exe
```

### 检查权限

确保 Package.appxmanifest 包含:
```xml
<Capabilities>
    <Capability Name="codeGeneration" />
</Capabilities>
```

---

## 📝 故障排除

### 问题 1: 无法连接到远程计算机

**解决**:
- 检查设备 IP: 192.168.3.67
- 确保设备开发者模式已启用
- 检查防火墙设置

### 问题 2: 部署失败

**解决**:
- 卸载设备上的旧版本
- 检查证书是否有效
- 尝试重启设备

### 问题 3: 应用崩溃

**解决**:
- 检查 codeGeneration 权限
- 查看事件查看器日志
- 使用远程调试查看错误

---

## 🎯 成功标准

### Phase 4 完成标准

- [ ] 应用成功部署到设备
- [ ] 成功加载 test_module.tccmod
- [ ] 符号解析正确（LSB = 1）
- [ ] 内存分配成功
- [ ] 内存保护设置正确
- [ ] 没有崩溃或错误

### 额外目标

- [ ] 实际执行 test_function (返回 42)
- [ ] 性能测试
- [ ] 多模块测试

---

## 📚 相关文档

- `START_HERE.md` - 下次会话指南
- `DEVICE_CONNECTED.md` - 设备连接信息
- `TCCMOD_FORMAT.md` - 文件格式规范
- `QUICK_REFERENCE.md` - API 参考

---

## ✅ 下一步

1. **使用 Visual Studio 打开项目**
2. **配置远程调试**
3. **部署到 Lumia 950 XL**
4. **运行测试**
5. **记录结果**

---

**准备就绪！可以开始部署和测试了！** 🚀

---

**创建时间**: 2026-07-27 00:15 UTC+8  
**状态**: Phase3Probe 项目文件已创建，等待 Visual Studio 构建

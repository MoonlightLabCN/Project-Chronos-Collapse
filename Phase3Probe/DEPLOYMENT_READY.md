# Phase3Probe - 部署完成

## ✅ 编译成功！

**包文件**: `Phase3Probe_1.0.0.0_ARM.appx`  
**位置**: `E:\Desktop\项目\UWP\Project-Chronos-Collapse\Phase3Probe\AppPackages\Phase3Probe\Phase3Probe_1.0.0.0_ARM_Test\`  
**大小**: 115,081 字节  
**目标设备**: Lumia 950 XL (192.168.3.67)

---

## 📱 部署步骤

### 方法 1: 使用 WinAppDeployCmd（推荐）

1. **在设备上启用开发者模式**：
   - 设置 > 更新和安全 > 开发者选项
   - 启用"开发人员模式"
   - 启用"设备门户"
   - 记下显示的配对 PIN 码

2. **运行部署命令**：
   ```powershell
   & "C:\Program Files (x86)\Windows Kits\10\bin\10.0.15063.0\x86\WinAppDeployCmd.exe" install -file "E:\Desktop\项目\UWP\Project-Chronos-Collapse\Phase3Probe\AppPackages\Phase3Probe\Phase3Probe_1.0.0.0_ARM_Test\Phase3Probe_1.0.0.0_ARM.appx" -ip 192.168.3.67 -pin <YOUR_PIN>
   ```
   
   将 `<YOUR_PIN>` 替换为设备上显示的 PIN 码。

3. **首次配对后**，之后部署不再需要 PIN：
   ```powershell
   & "C:\Program Files (x86)\Windows Kits\10\bin\10.0.15063.0\x86\WinAppDeployCmd.exe" install -file "E:\Desktop\项目\UWP\Project-Chronos-Collapse\Phase3Probe\AppPackages\Phase3Probe\Phase3Probe_1.0.0.0_ARM_Test\Phase3Probe_1.0.0.0_ARM.appx" -ip 192.168.3.67
   ```

### 方法 2: 手动安装

1. 将整个 `Phase3Probe_1.0.0.0_ARM_Test` 文件夹复制到设备
2. 在设备上双击 `.appx` 文件
3. 按照提示安装

### 方法 3: 使用 Visual Studio

1. 打开 Visual Studio 2022
2. 创建新的 UWP C++/CX 项目
3. 导入 Phase3Probe 所有文件
4. 配置远程调试：
   - 项目属性 > 调试
   - 远程计算机: 192.168.3.67
   - 身份验证类型: 无
5. 选择 ARM + Release，点击"部署"

---

## 📦 包内容

- **Phase3Probe.exe** - 主程序
- **Assets/test_module.tccmod** (161 字节) - ARM Thumb-2 测试模块
- **Assets/*.png** - 应用图标
- **tccmod_loader.dll** - .tccmod 运行时加载器

---

## 🎯 应用功能

1. **加载测试模块** - 从 Assets 加载 test_module.tccmod
2. **测试符号解析** - 查找 `test_function` 符号并验证地址
3. **验证 Thumb-2** - 检查函数指针 LSB=1

---

## ✅ 已验证

- ✅ 编译成功（ARM Release）
- ✅ 包签名完成
- ✅ 设备在线 (192.168.3.67)
- ✅ test_module.tccmod 包含在包中
- ✅ codeGeneration 权限已启用
- ✅ 所有资源文件就绪

---

## 🔧 技术细节

### 编译配置
- **平台**: ARM
- **配置**: Release
- **SDK**: 10.0.22621.0
- **最小版本**: 10.0.10240.0
- **工具集**: v143 (VS 2022)

### 关键功能
- UWP 内存管理 (VirtualAllocFromApp)
- W^X 内存保护 (VirtualProtectFromApp)
- ARM Thumb-2 函数指针 (LSB=1)
- 异步文件加载 (Windows::Storage)

---

## 📝 下一步

部署成功后：
1. 在设备上启动 Phase3Probe
2. 点击"加载测试模块"
3. 点击"测试符号解析"
4. 查看日志输出

预期结果：
```
+ Module loaded successfully!
Module info:
  - Magic: 0x4D434354 (TCCM)
  - Version: 1.0
  - Arch: ARM Thumb-2

+ Symbol address: 0x********1
+ LSB = 1 (Thumb-2 mode OK)
```

---

**状态**: 🟢 就绪，等待部署  
**设备**: Lumia 950 XL (在线)  
**包**: Phase3Probe_1.0.0.0_ARM.appx (112 KB)

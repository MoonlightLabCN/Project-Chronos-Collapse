# SandboxProbe 编译和部署指南

## 编译步骤

### 方法 1: Visual Studio（推荐）

1. **打开解决方案**
   ```
   双击打开: SandboxProbe.sln
   ```

2. **配置目标**
   - 配置: Release
   - 平台: ARM
   - 目标设备: 远程计算机

3. **配置远程设备**
   - 项目 → 属性 → 调试
   - 目标设备: 远程计算机
   - 远程计算机: 192.168.3.67
   - 身份验证模式: 通用(未加密协议)

4. **编译**
   - 生成 → 重新生成解决方案
   - 或按 Ctrl+Shift+B

5. **部署到设备**
   - 调试 → 开始执行(不调试) (Ctrl+F5)
   - 或 右键项目 → 部署

---

### 方法 2: 命令行编译

1. **打开 Visual Studio Developer Command Prompt**
   ```
   开始菜单 → Visual Studio 2022 → Developer Command Prompt for VS 2022
   ```

2. **编译**
   ```cmd
   cd "E:\Desktop\项目\UWP\Project-Chronos-Collapse"
   msbuild SandboxProbe.sln /t:Rebuild /p:Configuration=Release /p:Platform=ARM
   ```

3. **输出位置**
   ```
   ARM\Release\SandboxProbe\SandboxProbe.exe
   ```

---

## 部署到 Lumia 950 XL

### 前提条件

1. **设备设置**
   - 已启用开发者模式
   - Device Portal 已开启
   - IP 地址: 192.168.3.67

2. **电脑设置**
   - 在同一网络
   - 可以 ping 通 192.168.3.67

### 部署方法

#### 方法 A: Visual Studio 自动部署（推荐）
- 在 Visual Studio 中按 F5 或 Ctrl+F5
- 应用会自动部署并启动

#### 方法 B: 手动部署 .appx
1. 生成 .appx 包:
   ```
   项目 → 商店 → 创建应用包
   ```

2. 使用 Device Portal 上传:
   ```
   打开浏览器: http://192.168.3.67
   Apps → Install App → 选择 .appx 文件
   ```

---

## 验证安装

1. 在 Lumia 950 XL 上找到 "SandboxProbe" 应用
2. 启动应用
3. 应该看到 5 个测试按钮

---

## 测试顺序

### 快速验证
```
1. 点击 "Test 1: Basic JIT"
   → 应该显示 "TEST 1 PASSED ✓"

2. 点击 "Test 5: Full Test"
   → 运行所有测试并查看结果
```

### 详细测试
```
1. Test 1: Basic JIT       → 验证 JIT 能力
2. Test 2: Module Load     → 加载 .tccmod
3. Test 3: Execute         → 执行模块函数
4. Test 4: Host Call       → 测试回调
5. Clear Logs              → 清空日志
```

---

## 常见问题

### Q: 编译失败 "找不到 Windows SDK"
**A**: 确保安装了 Windows 10 SDK (10.0.17763.0 或更高)

### Q: 部署失败 "无法连接到远程设备"
**A**: 
- 检查设备 IP 是否正确
- 确认 Device Portal 已开启
- 尝试在浏览器打开 http://192.168.3.67

### Q: 应用崩溃
**A**: 
- 检查 Package.appxmanifest 中是否有 codeGeneration capability
- 查看 Visual Studio 输出窗口的错误信息

### Q: "test_compile.tccmod" 文件未找到
**A**: 
- 在 Visual Studio 解决方案资源管理器中找到文件
- 右键 → 属性 → 内容 = 是
- 重新生成项目

---

## 文件检查清单

在部署前确认：

- [ ] `test_compile.tccmod` 存在于 SandboxProbe 项目中
- [ ] `test_compile.tccmod` 的属性设置为 Content
- [ ] Package.appxmanifest 包含 `<Capability Name="codeGeneration" />`
- [ ] 项目配置为 ARM + Release
- [ ] 远程设备 IP 配置正确

---

## 下一步

### 如果测试全部通过
✅ 恭喜！基础设施已验证，可以开始：
1. TinyCC ARM32 Backend 开发
2. 完整的 .tccmod 编译流程
3. Python/JSC 集成

### 如果有测试失败
📋 收集以下信息：
1. 完整的测试日志（复制所有文本）
2. 哪个测试失败了
3. 失败时的错误信息
4. 设备信息（Test 1 会显示）

---

## 技术支持

需要帮助时提供：
- 测试日志截图或文本
- 编译输出（如果编译失败）
- 设备信息

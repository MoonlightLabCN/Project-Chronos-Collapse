# 如何生成和部署 SandboxProbe.appx

代码已经重写完成，现在需要编译和打包。有三种方法：

---

## 方法 1: Visual Studio 直接部署 ⭐ 推荐

这是**最简单**的方法，不需要手动打包。

### 步骤：

1. **打开项目**
   ```
   双击: SandboxProbe.sln
   ```

2. **配置**
   - 顶部工具栏选择: `Release` | `ARM`
   - 右键 SandboxProbe 项目 → 属性 → 调试
   - 目标设备: 远程计算机
   - 远程计算机名称: `192.168.3.67`
   - 身份验证模式: 通用(未加密协议)

3. **部署**
   - 右键 SandboxProbe 项目 → 部署
   - 或按 `Ctrl+F5` (开始执行不调试)

4. **完成**
   - 应用会自动安装到 Lumia 950 XL
   - 在设备上启动 SandboxProbe 应用

---

## 方法 2: 使用脚本编译 ⚡ 快速

如果你习惯命令行操作。

### 步骤：

1. **打开 Developer Command Prompt**
   ```
   开始菜单 → Visual Studio 2022 → Developer Command Prompt for VS 2022
   ```

2. **进入项目目录**
   ```cmd
   cd "E:\Desktop\项目\UWP\Project-Chronos-Collapse"
   ```

3. **运行构建脚本**
   ```cmd
   quick_build.bat
   ```

4. **编译完成后**
   - 编译输出在: `ARM\Release\SandboxProbe\`
   - 但这不是 .appx 包，只是构建产物
   - 仍需使用 Visual Studio 部署

---

## 方法 3: 生成独立 .appx 包 📦 完整打包

生成可以独立分发的 .appx 包。

### 步骤：

1. **在 Visual Studio 中打开项目**

2. **创建应用包**
   ```
   项目 → 商店 → 创建应用包
   → 选择 "我想创建用于旁加载的程序包"
   → 输出位置: 选择一个文件夹
   → 平台: ARM
   → 配置: Release
   → 点击 "创建"
   ```

3. **生成的文件**
   ```
   AppPackages\SandboxProbe\SandboxProbe_1.0.0.0_ARM_Test\
   ├── SandboxProbe_1.0.0.0_ARM.appx          ← 这个是安装包
   ├── SandboxProbe_1.0.0.0_ARM.cer           ← 证书
   └── Add-AppDevPackage.ps1                   ← 安装脚本
   ```

4. **部署到设备**

   **方法 A: 使用 Device Portal**
   ```
   1. 浏览器打开: http://192.168.3.67
   2. Apps → Install App
   3. 选择 .appx 文件上传
   4. 点击 Install
   ```

   **方法 B: 使用 PowerShell 脚本**
   ```powershell
   # 在电脑上执行（需要先配对设备）
   cd AppPackages\SandboxProbe\SandboxProbe_1.0.0.0_ARM_Test
   .\Add-AppDevPackage.ps1
   ```

---

## 为什么之前的 .appx 文件不见了？

之前的 appx 包是基于旧代码编译的。现在代码已重写，需要重新编译生成新的包。

**旧代码的问题**：
- 测试逻辑混乱
- 缺少清晰的测试结构
- 难以调试

**新代码的优势**：
- 5 个独立测试
- 清晰的日志系统
- 完整的错误处理
- 易于维护

---

## 快速开始 🚀

**最快的方式（推荐）**：

1. 打开 `SandboxProbe.sln`
2. 选择 `Release | ARM`
3. 按 `Ctrl+F5`
4. 等待部署完成
5. 在设备上启动应用
6. 点击 "Test 5: Full Test"
7. 把日志发给我

**需要时间**: 3-5 分钟

---

## 检查清单 ✅

编译前确认：

- [ ] Visual Studio 已安装
- [ ] Windows 10 SDK 已安装
- [ ] 设备 (192.168.3.67) 可访问
- [ ] Device Portal 已开启
- [ ] `test_compile.tccmod` 属性为 Content
- [ ] Package.appxmanifest 包含 codeGeneration capability

---

## 常见问题

### Q: 编译报错找不到 SDK
**A**: 安装 Windows 10 SDK 17763 或更高版本

### Q: 部署失败无法连接
**A**: 
```
1. 确认设备 IP: 192.168.3.67
2. 浏览器测试: http://192.168.3.67
3. 检查防火墙
```

### Q: 我只想要 .appx 文件
**A**: 使用方法 3，生成独立安装包

---

## 我的建议 💡

**直接用 Visual Studio 部署（方法 1）**

原因：
1. ✅ 最简单，3 步完成
2. ✅ 自动处理依赖
3. ✅ 自动签名
4. ✅ 可以调试
5. ✅ 不需要手动打包

**只有在需要分发给其他人时才生成 .appx 包（方法 3）**

---

**现在就开始吧！** 🚀

打开 Visual Studio → 打开 SandboxProbe.sln → 按 Ctrl+F5

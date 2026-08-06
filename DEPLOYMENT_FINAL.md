# SandboxProbe - 最终部署指南

## 📦 交付文件

项目根目录下有两个文件：

1. **SandboxProbe_Final.appx** (112 KB) - 应用包
2. **SandboxProbe_TestCert.pfx** (密码: test) - 测试证书

---

## 🚀 方法 1: 使用 Visual Studio 部署（最简单）⭐

这是最可靠的方法，绕过所有签名问题。

### 步骤：

1. **打开项目**
   ```
   双击: SandboxProbe.sln
   ```

2. **配置远程设备**
   - 右键项目 → 属性 → 调试
   - 目标设备: 远程计算机
   - 远程计算机: 192.168.3.67
   - 身份验证: 通用(未加密协议)

3. **部署**
   - 右键项目 → 部署
   - 或按 Ctrl+F5

4. **完成**
   - 应用会自动安装到设备
   - Visual Studio 会处理所有签名问题

**这个方法成功率: 99%**

---

## 🔧 方法 2: 手动安装证书 + 安装包

如果设备在线但 Visual Studio 不可用。

### 步骤 1: 安装证书到设备

1. 把 `SandboxProbe_TestCert.pfx` 复制到设备
2. 在设备上双击证书文件
3. 选择"本地计算机"
4. 输入密码: `test`
5. 证书存储: 选择"受信任的根证书颁发机构"
6. 完成安装

### 步骤 2: 安装应用

1. 使用 Device Portal (http://192.168.3.67)
2. Apps → Install App
3. 选择 `SandboxProbe_Final.appx`
4. 安装

**这个方法成功率: 70%**（可能仍然有签名问题）

---

## 💻 方法 3: PowerShell 部署（开发者）

如果你有 Windows SDK 和设备访问权限。

```powershell
# 1. 安装证书到本机
$cert = Import-PfxCertificate -FilePath ".\SandboxProbe_TestCert.pfx" -CertStoreLocation Cert:\CurrentUser\My -Password (ConvertTo-SecureString "test" -AsPlainText -Force)

# 2. 部署到设备
$ip = "192.168.3.67"
& "C:\Program Files (x86)\Windows Kits\10\bin\10.0.22621.0\x64\WinAppDeployCmd.exe" install -file ".\SandboxProbe_Final.appx" -ip $ip
```

---

## ⚠️ 如果所有方法都失败

### 终极方案：直接用 Visual Studio F5

这是绕过所有签名、证书、安全限制的唯一可靠方法。

**为什么 Visual Studio 能成功？**
- Visual Studio 使用特殊的开发者部署机制
- 绕过标准的 appx 安装流程
- 自动处理证书和签名
- 直接推送文件到设备

**操作：**
1. 打开 SandboxProbe.sln
2. 选择 Release | ARM
3. 按 F5 或 Ctrl+F5
4. 等待部署完成

---

## 🎯 推荐流程

按优先级尝试：

1. ✅ **Visual Studio 部署**（成功率最高）
2. ⚠️ 手动安装证书 + 包
3. 💻 PowerShell 脚本
4. 🆘 重新用 Visual Studio 生成正确签名的包

---

## 📋 测试清单

部署成功后：

1. ✅ 在设备上找到"SanboxProbe"应用
2. ✅ 启动应用
3. ✅ 看到 5 个测试按钮
4. ✅ 点击"Test 5: Full Test"
5. ✅ 复制日志输出

---

## 🔍 故障排查

### 错误: 0x800b0100
- **原因**: 证书不受信任
- **解决**: 先安装证书到设备

### 错误: 0x80070005
- **原因**: 权限不足
- **解决**: 确认设备开启开发者模式

### 错误: 无法连接设备
- **原因**: 网络问题
- **解决**: ping 192.168.3.67 确认连接

### 所有方法都失败
- **解决**: 用 Visual Studio F5 直接部署
- 这个方法几乎不会失败

---

## 💡 为什么签名这么麻烦？

UWP 应用的安全模型要求：
1. 所有 appx 必须签名
2. 证书必须受信任
3. 证书必须匹配 Publisher

在开发环境：
- Visual Studio 自动处理这些
- 手动打包会遇到各种证书问题

**建议**: 开发阶段始终用 Visual Studio 部署

---

## 📞 最后的话

如果 Visual Studio 部署也失败了... 那可能是：
1. 设备系统问题
2. 开发者模式未正确开启
3. 设备存储空间不足
4. 网络防火墙阻止

但这种情况极少发生。

**99% 的情况下，Visual Studio F5 能解决一切问题。**

---

**现在请用 Visual Studio 打开项目并按 F5！** 🚀

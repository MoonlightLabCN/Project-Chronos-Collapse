# 📱 Lumia 950 XL 文件推送指南

## 🔍 连接诊断结果

**设备 IP**: 192.168.3.18  
**WDP 端口**: 50443

### 端口状态
- ✅ 端口 443 开放
- ✅ 端口 50443 开放
- ✅ 端口 80 开放
- ✅ 端口 8080 开放

### 连接问题
- ❌ Ping 失败（正常，防火墙阻止）
- ❌ HTTPS 连接超时

---

## 🛠️ 解决方案

### 方案 1: 检查 Device Portal 配置

1. **在 Lumia 950 XL 上**:
   ```
   设置 -> 更新和安全 -> 针对开发人员
   ```

2. **启用开发者模式**:
   - 选择"开发人员模式"
   - 等待安装完成

3. **启用 Device Portal**:
   - 打开"设备门户"
   - 设置身份验证（PIN 或用户名/密码）
   - 记下显示的 URL（例如: https://192.168.3.18:443）

4. **在电脑浏览器中测试**:
   ```
   https://192.168.3.18:443
   或
   https://192.168.3.18:50443
   ```
   - 应该能看到 Device Portal 登录界面
   - 接受自签名证书警告

---

### 方案 2: 使用浏览器手动上传

如果能打开 Device Portal 网页界面：

1. **访问文件浏览器**:
   ```
   https://192.168.3.18:443/fileexplorer.htm
   或
   https://192.168.3.18:50443/#FileExplorer
   ```

2. **导航到目标目录**:
   ```
   LocalAppData -> 创建文件夹 "Phase3Probe"
   ```

3. **手动上传以下文件**:
   - `tinycc/win32/build/test_module.tccmod` (161 bytes) ⭐ 最重要
   - `tinycc/tccmod.h` (8.0 KB)
   - `tinycc/tccmod_loader.c` (18 KB)
   - `TCCMOD_FORMAT.md` (格式规范)
   - `QUICK_REFERENCE.md` (快速参考)

---

### 方案 3: 使用 PowerShell + curl

如果 WDP 可访问但脚本失败，使用这个命令：

```powershell
# 测试连接
curl -k https://192.168.3.18:443/api/os/info

# 如果上面成功，推送文件
$file = "tinycc/win32/build/test_module.tccmod"
$url = "https://192.168.3.18:443/api/filesystem/apps/file?knownfolderid=LocalAppData&filename=Phase3Probe/test_module.tccmod"

# 无密码
curl -k -X PUT -H "Content-Type: application/octet-stream" --data-binary "@$file" $url

# 有密码
curl -k -u "Administrator:YourPIN" -X PUT -H "Content-Type: application/octet-stream" --data-binary "@$file" $url
```

---

### 方案 4: 通过 UWP 应用部署

**最可靠的方法**：

1. 创建一个简单的 UWP 应用（Phase3Probe）
2. 将文件添加为应用资源或内容
3. 通过 Visual Studio 部署到设备
4. 应用可以访问自己的 LocalAppData

参考 `SandboxProbe` 项目的部署方式。

---

## 📦 需要推送的文件清单

### 高优先级（必需）
1. **test_module.tccmod** (161 bytes)
   - 位置: `tinycc/win32/build/test_module.tccmod`
   - 目标: `LocalAppData/Phase3Probe/test_module.tccmod`
   - 用途: 测试加载器功能

### 中优先级（有用）
2. **tccmod.h** (8.0 KB)
   - 数据结构定义
   
3. **tccmod_loader.c** (18 KB)
   - 加载器源码

4. **TCCMOD_FORMAT.md**
   - 格式规范文档

5. **QUICK_REFERENCE.md**
   - 快速参考指南

---

## 🔧 调试步骤

### 1. 确认 Device Portal 可访问

```bash
# 在 Git Bash 或 WSL 中
curl -k https://192.168.3.18:443
curl -k https://192.168.3.18:50443
curl -k https://192.168.3.18:443/api/os/info
```

**预期结果**: 应该返回 HTML 或 JSON 响应

### 2. 检查设备 IP

在 Lumia 950 XL 上:
```
设置 -> 网络和无线 -> WLAN -> 高级 -> 查看 IP 地址
```

确认 IP 是 192.168.3.18

### 3. 检查防火墙

在电脑上暂时禁用防火墙测试：
```powershell
# 查看防火墙状态
Get-NetFirewallProfile | Select Name, Enabled

# 临时禁用（测试后记得启用）
Set-NetFirewallProfile -Profile Domain,Public,Private -Enabled False
```

---

## 💡 推荐方案

**最简单的方法**：

1. 在浏览器中打开: `https://192.168.3.18:443`
2. 登录 Device Portal（接受证书警告）
3. 进入 File Explorer
4. 手动上传 `test_module.tccmod`

**如果浏览器也无法访问**：

- 设备可能未正确配置 Device Portal
- 检查设备的开发者设置
- 重启设备和 Device Portal
- 确认设备和电脑在同一网络

---

## 📝 下一步

一旦文件成功推送到设备：

1. 创建 Phase3Probe UWP 应用
2. 在应用中集成 `tccmod_loader.c`
3. 加载并测试 `test_module.tccmod`
4. 验证 ARM Thumb-2 代码执行

---

## ❓ 需要帮助？

**如果仍然无法连接**：

1. 截图设备的开发者设置页面
2. 截图 Device Portal 显示的 URL
3. 在浏览器中访问该 URL 并截图结果
4. 检查设备和电脑的网络配置

**文件位置参考**：

- 电脑上: `E:\Desktop\项目\UWP\Project-Chronos-Collapse\tinycc\win32\build\test_module.tccmod`
- 设备上: `C:\Data\Users\DefaultAccount\AppData\Local\Phase3Probe\test_module.tccmod`

---

**创建时间**: 2026-07-26 23:30 UTC+8

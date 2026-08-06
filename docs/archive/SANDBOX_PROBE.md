# SandboxProbe - Windows 10 Mobile JIT 验证报告 (Phase 1 总结)

本文件是 **SandboxProbe** 项目（Phase 1 阶段）的完整合订文档，汇总了在 Windows 10 Mobile (ARM32, Lumia 950 XL) 平台上验证 JIT 支持的全部发现、技术规格、部署方法及排产记录。

---

## 🎯 核心目标与结论

### 目标
验证 Windows 10 Mobile (Lumia 950 XL) 是否支持 JIT (Just-In-Time) 编译与动态代码执行，探讨规避 UWP 应用代码完整性 (Code Integrity) 限制的可行性。

### 结论
✅ **成功验证：Windows 10 Mobile 完全支持 JIT 编译！**
在 UWP 应用中通过正确声明普通的 `codeGeneration` 权限，`VirtualProtectFromApp` 能够成功将分配的 RW 内存修改为 RX (PAGE_EXECUTE_READ) 权限，动态生成的 ARM Thumb-2 机器码执行正常，并成功返回预期值 `42`。

---

## 🔬 真实设备测试报告 (Lumia 950 XL)

### 1. 测试环境
* **测试日期**：2026年7月26日
* **测试人员**：Ouyang Quan
* **设备型号**：Microsoft Lumia 950 XL
* **处理器架构**：ARM (32-bit, ARMv7-A)
* **操作系统**：Windows 10 Mobile (OS Build 10.0.15254.603)
* **部署方式**：Windows Device Portal
* **最终测试应用**：SandboxProbe v3.0.0 (ARM Release with Fixed Capability)

### 2. 演进与突破过程
在测试期间，项目经历了多次尝试，最终通过对比 Apotheosis 成功项目的配置找到了关键突破口：

| 版本 | Capability 声明配置 | 结果 | 错误/状态 | 说明 |
| :--- | :--- | :--- | :--- | :--- |
| **v1.0** | `<rescap:Capability Name="codeGeneration" />` | ❌ 失败 | 5 (ACCESS_DENIED) | 错误使用限制级 `rescap:` 命名空间 |
| **v2.0** | 无任何 `codeGeneration` 声明 | ❌ 失败 | 5 (ACCESS_DENIED) | 权限转换 API 被系统彻底阻止 |
| **v3.0** | `<Capability Name="codeGeneration" />` | ✅ **成功** | N/A (返回 42) | **关键突破**：必须配置为普通 Capability |

> [!IMPORTANT]
> **关键发现**：微软官方 UWP 权限文档可能带有误导性，暗示 `codeGeneration` 属于受限命名空间 `rescap`。然而在 Windows 10 Mobile 上，它必须作为**普通的 Capability**（无前缀）声明，否则 `VirtualProtectFromApp` 会无条件返回 Access Denied (Error 5)。

### 3. 测试输出日志 (Test 1 成功运行)
```
Starting Test 1: Dynamic JIT Memory Execution
Detected Host Architecture: ARM (32-bit)
1. Allocating read-write virtual memory via VirtualAllocFromApp...
Allocated RW page at address: 0x00b50000
2. Copying target machine code bytes into memory...
3. Converting page protection to PAGE_EXECUTE_READ via VirtualProtectFromApp...
Page protection changed successfully.
Running ARM32: Tagging target address LSB with 1 (Thumb-2 mode).
4. Jumping to function address: 0x00b50001
SUCCESS: JIT execution succeeded!
Returned value (Expected 42): 42
>>> TEST 1 PASSED <<<
Cleaned up virtual memory pages.
```

---

## 🛠️ 技术规格与实现细节

### 1. 内存管理逻辑 (W^X 隔离策略)
Windows 10 Mobile 的安全机制强制执行 **W^X (Write XOR Execute)** 策略：
* 禁止分配同时具备“可写”与“可执行”属性的内存（如 `PAGE_EXECUTE_READWRITE` 将分配失败，错误代码 87）。
* 必须遵循以下时序逻辑：
  1. 调用 `VirtualAllocFromApp` 分配 RW (`PAGE_READWRITE`) 属性的内存页。
  2. 拷贝机器码字节序列入该内存页，并调用 `FlushInstructionCache` 刷新缓存确保硬件指令一致性。
  3. 调用 `VirtualProtectFromApp` 将内存保护属性修改为 RX (`PAGE_EXECUTE_READ`)。
  4. 跳转执行。

### 2. ARM Thumb-2 机器码注入与状态标记 (LSB = 1)
测试所用的 Thumb-2 汇编指令如下：
```asm
movs r0, #42    ; 将立即数 42 加载到返回值寄存器 r0 (机器码: 0x2A, 0x20)
bx   lr         ; 返回指令 (机器码: 0x70, 0x47)
```
**Thumb-2 状态标记重要限制**：
在 32 位 Windows ARM 平台中，用户态代码必须运行在 Thumb 状态。跳转到分配 the 内存段地址执行时，**函数指针的最低有效位 (LSB) 必须被设为 1**：
```cpp
uintptr_t targetAddr = (uintptr_t)memory | 1u;
jit_fn_t jitFunc = (jit_fn_t)targetAddr;
```
若不将 LSB 设为 1，CPU 会尝试以 ARM32 经典指令模式解码，这在 Windows 10 Mobile 上会被 DEP 判定为非法指令而直接触发应用崩溃（异常 `0xC000001D`）。

### 3. SEH (结构化异常处理) 规避 C++/CX 展开冲突
由于 JIT 操作属于低级内存操作，易触发 CPU 异常（如 `0xC0000005` 内存访问冲突）。
* 在 C++/CX 项目中，`__try / __except` 块内如果存在需要析构的 C++ 局部对象，编译器会报错 `C2712: 无法在易于发生异常的函数中使用 __try`。
* **解决方案**：将低级执行逻辑剥离至独立的 C 风格包装函数中：
```cpp
static int ExecuteJitFunction(jit_fn_t jitFunc, DWORD* exceptionCode)
{
    __try {
        return jitFunc();
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        *exceptionCode = GetExceptionCode();
        return -1;
    }
}
```

---

## 📦 部署与打包指南

### 1. 签名与证书匹配问题 (0x8007000b)
在早期打包尝试中，经常会遇到 APPX 签名错误：`SignerSign() failed. (0x8007000b/ERROR_BAD_FORMAT)`。
* **原因**：自签名证书的 `Subject` 与 `Package.appxmanifest` 中的 `Identity Publisher` 字符串必须百分百一致（包括标点符号、空格与大小写）。
* **解决**：在系统中安装或生成 `CN=Ouyang Quan` 的匹配证书，并导出了签名证书 `SandboxProbe_Matching.pfx`，从而使用 `SignTool` 成功签发了包。

### 2. 部署方式
推荐直接使用 **Windows Device Portal**（在同一 Wi-Fi 网络下访问设备 IP）。
* **包名**：`SandboxProbe_ARM_Release_FixedCapability.appx`（已签名并验证成功的包）。
* **证书部署**：如果设备未配置信任，可以通过 Portal 上传 `SandboxProbe.cer` 并安装到设备的 **“受信任的根证书颁发机构”** 中，随后直接安装 `.appx` 包。
* **脚本部署**：可以直接通过 PowerShell 脚本进行部署：
  ```powershell
  .\deploy.ps1 -DeviceIP <设备IP>
  ```

---

## 👥 平台对比

| 平台 | JIT 支持情况 | 配置要求 | 限制机制 |
| :--- | :--- | :--- | :--- |
| **Windows 10 Mobile** | ✅ **支持** | `<Capability Name="codeGeneration" />` | 强制 W^X 策略；仅支持普通权限豁免，限制级命名空间无效。 |
| **Windows 10 Desktop UWP** | ✅ **支持** | `<Capability Name="codeGeneration" />` | 同样遵循 W^X。 |
| **iOS** | ⚠️ **部分支持** | 需配置特定 Entitlement | 仅限系统级 JavaScriptCore，非越狱一般应用无法使用。 |
| **Android** | ✅ **支持** | 无特殊要求 | 相对开放，一般应用默认允许动态生成执行。 |

---

## 📅 项目时间线与交付成果

### 里程碑
* **2026-07-26 14:00** - 项目启动
* **2026-07-26 17:42** - Release 编译成功
* **2026-07-26 18:12** - 数字证书与 APPX 签名完成
* **2026-07-26 18:30** - JIT 测试初次因 ACCESS_DENIED 失败，定位到 `rescap:Capability` 配置错误
* **2026-07-26 20:30** - 修正 Capability 重新打包，Lumia 950 XL 上运行 Test 1 通过，返回 `42`，Phase 1 完美收官！

### 交付物理资产
* `SandboxProbe_ARM_Release_FixedCapability.appx` (1.28 MB) - 最终已签名且可运行的 APPX 包。
* `SandboxProbe.cer` - 设备安装用证书。
* `SandboxProbe_Matching.pfx` - 签名证书（私钥密码: `test123`）。
* `deploy.ps1` - 自动化部署脚本。

# SandboxProbe - 测试指南

## 概述

这是一个全新重写的测试程序，专为验证 Windows 10 Mobile ARM32 上的动态代码执行能力而设计。

## 测试架构

```
Test 1: Basic JIT
    ↓
Test 2: Module Load
    ↓
Test 3: Execute
    ↓
Test 4: Host Call
    ↓
Test 5: Full Integration
```

---

## 测试说明

### Test 1: Basic JIT Execution
**目标**: 验证最基础的动态代码执行能力

**测试内容**:
- 使用 `VirtualAllocFromApp` 分配内存
- 复制 ARM Thumb-2 机器码（返回 42 的函数）
- 使用 `VirtualProtectFromApp` 转换为可执行
- 设置 LSB=1（Thumb-2 模式）
- 执行并验证返回值

**预期结果**:
- ✅ 内存分配成功
- ✅ 保护属性修改成功
- ✅ 执行成功，返回 42

**如果失败**:
- 检查 `codeGeneration` capability 是否正确配置
- 确认是在 ARM32 设备上运行

---

### Test 2: TccMod Module Loading
**目标**: 验证 .tccmod 文件加载和解析

**测试内容**:
- 从应用包加载 `test_compile.tccmod`
- 解析文件头、段表、符号表
- 验证模块结构完整性

**预期结果**:
- ✅ 文件读取成功
- ✅ 格式验证通过
- ✅ 模块保存到内存

**如果失败**:
- 确认 `test_compile.tccmod` 在项目中标记为 Content
- 检查文件格式是否正确（magic number 等）

---

### Test 3: Execute Module Function
**目标**: 执行模块中的简单函数

**前提**: 必须先运行 Test 2

**测试内容**:
- 链接模块（应用重定位）
- 查找符号 `test_function` 或 `calculate`
- 检查 LSB 标记
- 执行函数并获取返回值

**预期结果**:
- ✅ 链接成功
- ✅ 符号找到
- ✅ LSB = 1
- ✅ 执行成功，返回预期值

**如果失败**:
- 检查模块是否包含目标符号
- 验证 LSB 设置是否正确
- 查看异常代码判断崩溃原因

---

### Test 4: Host Function Callback
**目标**: 验证模块调用宿主函数

**前提**: 必须先运行 Test 2

**测试内容**:
- 注册宿主符号 `host_add` 和 `host_print`
- 链接模块（解析外部符号）
- 执行 `calculate(10, 32)` 函数
- 验证宿主函数被正确调用

**预期逻辑**:
```
calculate(10, 32):
  1. 调用 host_add(10, 32) → 返回 42
  2. 计算 square(42) → 返回 1764
  3. 返回 1764
```

**预期结果**:
- ✅ 宿主符号注册成功
- ✅ 符号解析成功
- ✅ 执行时调用宿主函数
- ✅ 返回值 = 1764

**如果失败**:
- 检查模块是否使用了 `host_add`
- 验证重定位表是否正确
- 确认 ABI 调用约定正确

---

### Test 5: Full Integration Test
**目标**: 按顺序运行所有测试

**测试内容**:
- 依次执行 Test 1-4
- 生成完整测试报告

**用途**: 
- 快速验证所有功能
- 生成完整的测试日志

---

## 当前模块状态

### test_compile.tccmod
根据项目结构，应该包含以下源码编译结果：

**源文件**: `tinycc/win32/test_compile.c`

**导出符号**:
- `calculate(int a, int b)` - 主测试函数
- `get_counter()` - 获取计数器
- `greet()` - 调用 host_print
- `complex_calc(int x, int y, int z)` - 复杂计算

**外部依赖**:
- `host_add(int a, int b)` - 宿主提供的加法函数
- `host_print(const char* msg)` - 宿主提供的打印函数

---

## 如何测试

### 步骤 1: 按顺序测试
1. 点击 **Test 1: Basic JIT**
2. 等待结果（应该通过）
3. 点击 **Test 2: Module Load**
4. 点击 **Test 3: Execute**
5. 点击 **Test 4: Host Call**

### 步骤 2: 或直接运行完整测试
- 点击 **Test 5: Full Test**
- 查看所有测试结果

### 步骤 3: 分析结果
- `[✓]` = 成功
- `[✗]` = 失败
- `[i]` = 信息

---

## 故障排查

### Test 1 失败
**症状**: VirtualProtectFromApp 失败
**原因**: codeGeneration capability 未正确配置
**解决**: 
```xml
<Capabilities>
  <Capability Name="codeGeneration" />  <!-- 必须是普通 Capability -->
</Capabilities>
```

### Test 2 失败
**症状**: 文件未找到
**原因**: test_compile.tccmod 未包含在包中
**解决**: 
- 在 Visual Studio 中右键文件
- 属性 → 内容 = 是

### Test 3/4 失败
**症状**: 符号未找到
**原因**: test_compile.tccmod 不包含预期符号
**解决**: 
- 需要用 TCC 编译 test_compile.c 生成正确的 .tccmod 文件

### 执行崩溃
**症状**: Exception 0xC000001D (Illegal Instruction)
**原因**: LSB 未设置为 1
**解决**: 已在代码中自动处理

---

## 下一步

根据测试结果：

### 如果 Test 1-3 全部通过
✅ **结论**: ARM32 动态代码执行完全可行
📋 **行动**: 
- 开始开发 TinyCC ARM32 Thumb-2 backend
- 实现完整的 .tccmod 编译器集成

### 如果 Test 4 失败（其他通过）
⚠️ **结论**: 基础 JIT 可行，但重定位或符号解析有问题
📋 **行动**:
- 检查重定位表生成逻辑
- 验证 ARM Thumb-2 重定位类型
- 调试符号解析过程

### 如果 Test 1 失败
❌ **结论**: 系统限制动态代码执行
📋 **行动**:
- 考虑 WASM/JSC fallback 方案
- 或使用预编译 DLL + 动态调用

---

## 日志示例

### 成功的测试输出
```
========================================
TEST 1: Basic JIT Execution
========================================
[i] Testing dynamic code execution on ARM32 Thumb-2...
[i] Device: ARM (32-bit)
[i] Page Size: 4096 bytes
[i] Allocating RW memory...
[✓] Allocated at: 0x04a60000
[i] Copying machine code...
[✓] Code copied.
[i] Changing protection to PAGE_EXECUTE_READ...
[✓] Protection changed to RX.
[i] Executing JIT code...
[✓] Returned: 42 (expected 42)
[✓] TEST 1 PASSED ✓
[i] Memory freed.
```

---

## 文件清单

- `MainPage.xaml` - UI 布局（5 个测试按钮）
- `MainPage.xaml.h` - 头文件（接口定义）
- `MainPage.xaml.cpp` - 实现文件（所有测试逻辑）
- `tccmod.h` - TccMod 格式定义
- `tccmod_loader.c` - TccMod 运行时加载器
- `test_compile.tccmod` - 测试模块（需要包含在包中）

---

## 联系与支持

如有问题，请提供：
1. 完整的测试日志输出
2. 设备信息（从 Test 1 输出获取）
3. test_compile.tccmod 文件大小和生成方式

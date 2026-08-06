# Quick Reference Guide - TccMod System

## 🚀 快速开始

### 测试已实现的功能

```bash
# 1. 进入测试目录
cd tinycc/win32

# 2. 运行自动化测试
./build_test.bat

# 预期输出：
# ✅ 编译测试工具
# ✅ 生成 test_module.tccmod
# ✅ 测试加载器
# ✅ 所有测试通过
```

---

## 📁 关键文件速查

### 核心实现
| 文件 | 行数 | 功能 |
|------|------|------|
| `tccmod.h` | 250 | 数据结构和API定义 |
| `tccmod_loader.c` | 700+ | 运行时加载器 |
| `tccmod_builder.c` | 400+ | TCC序列化器 |

### 已修改的 TCC 文件
| 文件 | 修改内容 |
|------|----------|
| `libtcc.h` | 添加 `TCC_OUTPUT_TCCMOD` 和 `tcc_output_tccmod()` API |
| `tccelf.c` | 在 `tcc_output_file()` 中添加 .tccmod 路由 |
| `Makefile` | 将 tccmod_builder.c 和 tccmod.h 加入 CORE_FILES |

### 测试工具
| 文件 | 功能 |
|------|------|
| `create_test_module.c` | 手动生成测试 .tccmod 文件 |
| `test_simple_loader.c` | 验证加载器功能 |
| `test_compile.c` | 用于 TCC 编译测试 |
| `check_layout.c` | 验证结构体布局 |

---

## 🔧 核心 API

### 加载器 API

```c
// 加载 .tccmod 文件
TccModRuntime* tccmod_load(const char *filename, 
                            TccModErrorFunc error_func, 
                            void *opaque);

// 从内存加载
TccModRuntime* tccmod_load_memory(const void *buffer, size_t size,
                                   TccModErrorFunc error_func, 
                                   void *opaque);

// 添加宿主符号
int tccmod_add_symbol(TccModRuntime *mod, 
                      const char *name, 
                      const void *addr);

// 获取模块符号
void* tccmod_get_symbol(TccModRuntime *mod, const char *name);

// 获取入口点
void* tccmod_get_entry_point(TccModRuntime *mod);

// 卸载模块
void tccmod_unload(TccModRuntime *mod);
```

### TCC 集成 API

```c
// 在 TCC 中生成 .tccmod 文件
LIBTCCAPI int tcc_output_tccmod(TCCState *s, const char *filename);

// 使用示例：
TCCState *s = tcc_new();
tcc_set_output_type(s, TCC_OUTPUT_TCCMOD);
tcc_compile_string(s, "int add(int a, int b) { return a + b; }");
tcc_output_tccmod(s, "output.tccmod");
tcc_delete(s);
```

---

## 📊 .tccmod 文件格式

### 文件结构
```
Offset | Size | Content
-------|------|------------------
0x00   | 80   | Header
0x50   | var  | Section Table
...    | var  | Symbol Table
...    | var  | String Table
...    | var  | Relocation Table
...    | var  | Section Data
```

### 头部字段（80字节）
```c
typedef struct {
    uint32_t magic;              // 0x4D434354 ("TCCM")
    uint16_t version_major;      // 1
    uint16_t version_minor;      // 0
    uint32_t flags;              // TCCMOD_FLAG_THUMB2
    uint32_t timestamp;
    uint32_t checksum;
    uint32_t entry_point_rva;
    uint32_t section_table_offset;
    uint16_t section_count;
    uint16_t reserved1;
    uint32_t symbol_table_offset;
    uint32_t symbol_count;
    uint32_t string_table_offset;
    uint32_t string_table_size;
    uint32_t reloc_table_offset;
    uint32_t reloc_count;
    uint32_t import_table_offset;
    uint32_t import_count;
    uint32_t export_table_offset;
    uint32_t export_count;
    uint8_t  reserved2[8];
} TccModHeader;
```

---

## 🔍 调试技巧

### 检查 .tccmod 文件

```bash
# 查看文件大小
ls -lh test.tccmod

# 查看 hex dump（前 160 字节）
xxd -l 160 test.tccmod

# 验证 magic number
xxd -l 4 test.tccmod
# 应该输出: 54 43 43 4d (TCCM in little-endian)
```

### 检查结构体布局

```bash
# 运行布局检查工具
./build/check_layout.exe

# 预期输出：
# Total size: 80 bytes
# OK: Structure size is correct.
```

### 加载器调试

```bash
# 使用详细输出
./build/test_simple_loader.exe test.tccmod

# 检查输出：
# ✅ Magic number correct
# ✅ Module loaded
# ✅ Symbol found
```

---

## ⚠️ 常见问题

### Q1: 结构体大小不正确
**症状**: check_layout 显示大小 != 80 字节  
**原因**: 编译器添加了填充字节  
**解决**: 确保使用了 `#pragma pack(1)`

### Q2: Magic number 不匹配
**症状**: 加载器报告 "Invalid magic number"  
**原因**: 文件损坏或格式错误  
**解决**: 重新生成 .tccmod 文件

### Q3: 符号未找到
**症状**: `tccmod_get_symbol()` 返回 NULL  
**原因**: 
- 符号不是 global
- 符号名拼写错误
- 序列化器未包含该符号

**解决**: 
```c
// 确保函数声明为全局
int my_function(int x) { ... }  // ✅ global
static int my_func(int x) { ... }  // ❌ local
```

### Q4: ARM Thumb-2 代码执行失败
**症状**: 函数调用崩溃  
**原因**: LSB 未设置为 1  
**解决**: 加载器会自动处理，确保使用 `tccmod_get_symbol()`

---

## 🎯 测试检查清单

### 加载器测试
- [ ] 文件打开成功
- [ ] Magic number 验证通过
- [ ] 头部解析正确
- [ ] Section 加载成功
- [ ] 符号表解析正确
- [ ] 符号查找成功
- [ ] LSB 正确设置（Thumb-2）

### 序列化器测试
- [ ] 从 TCC 提取 sections
- [ ] 符号表转换正确
- [ ] 重定位信息完整
- [ ] 文件头正确写入
- [ ] 所有偏移量正确
- [ ] 文件大小合理

---

## 📚 参考文档

### 必读文档
1. `TCCMOD_FORMAT.md` - 完整格式规范
2. `PHASE3_REPORT.md` - 实现细节
3. `FINAL_STATUS.md` - 当前状态

### 有用的命令

```bash
# 编译 TCC（如果需要）
cd tinycc
./configure --cpu=arm --enable-cross
make

# 运行所有测试
cd win32
./build_test.bat

# 生成 .tccmod（TCC 编译后）
./arm-win32-tcc -o output.tccmod input.c

# 测试加载器
./build/test_simple_loader.exe output.tccmod
```

---

## 🚀 下一步

### 立即任务
1. 编译包含 tccmod 支持的 TCC
2. 测试 C 源码 → .tccmod 编译
3. 验证完整工作流

### 近期任务
1. 创建 UWP 测试应用
2. 集成到 Lumia 950 XL
3. 真机执行验证

### 长期任务
1. Phase 4 完善
2. Phase 5 Python 集成
3. 生产就绪优化

---

## 💻 代码示例

### 完整使用示例

```c
#include "tccmod.h"

void error_handler(void *opaque, const char *msg) {
    fprintf(stderr, "Error: %s\n", msg);
}

int host_function(int x) {
    return x * 2;
}

int main() {
    // 1. 加载模块
    TccModRuntime *mod = tccmod_load("module.tccmod", 
                                      error_handler, NULL);
    if (!mod) return 1;
    
    // 2. 注册宿主函数
    tccmod_add_symbol(mod, "host_function", host_function);
    
    // 3. 获取模块函数
    int (*calc)(int, int) = tccmod_get_symbol(mod, "calculate");
    if (!calc) {
        tccmod_unload(mod);
        return 1;
    }
    
    // 4. 执行
    int result = calc(10, 20);
    printf("Result: %d\n", result);
    
    // 5. 清理
    tccmod_unload(mod);
    return 0;
}
```

---

**最后更新**: 2026-07-26 22:20 UTC+8  
**维护者**: Project Chronos Collapse Team

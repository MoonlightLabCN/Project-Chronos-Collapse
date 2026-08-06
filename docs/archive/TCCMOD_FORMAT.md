# .tccmod Native Module Image Format Specification

## 概述

`.tccmod` 是专为 Project Chronos Collapse 设计的轻量级可序列化原生模块镜像格式。它允许将 TinyCC 编译的 ARM Thumb-2 代码保存为独立的模块文件，并在运行时动态加载到内存中执行，完全绕过 Windows UWP 的 PE 加载机制。

## 设计目标

1. **轻量化**：最小化文件头开销，紧凑的数据结构
2. **自包含**：包含所有必要的代码、数据、符号和重定位信息
3. **可验证**：包含校验和以确保完整性
4. **版本化**：支持格式演进和向后兼容性检查

## 文件结构

```
┌─────────────────────────────────────┐
│  File Header (64 bytes)             │
├─────────────────────────────────────┤
│  Section Table                      │
├─────────────────────────────────────┤
│  .text Section (Code)               │
│  - ARM Thumb-2 machine code         │
├─────────────────────────────────────┤
│  .data Section (Initialized Data)   │
├─────────────────────────────────────┤
│  .rodata Section (Read-Only Data)   │
├─────────────────────────────────────┤
│  .bss Section Descriptor            │
│  (no actual data, size only)        │
├─────────────────────────────────────┤
│  Symbol Table                       │
├─────────────────────────────────────┤
│  String Table                       │
├─────────────────────────────────────┤
│  Relocation Table                   │
├─────────────────────────────────────┤
│  Import Table                       │
├─────────────────────────────────────┤
│  Export Table                       │
└─────────────────────────────────────┘
```

## 数据结构定义

### 1. File Header (64 bytes)

```c
typedef struct {
    uint32_t magic;              // Magic number: 0x4D4F4443 ("TCCM" in little-endian)
    uint16_t version_major;      // Format version: 1
    uint16_t version_minor;      // Format version: 0
    uint32_t flags;              // Module flags (see below)
    uint32_t timestamp;          // Compilation timestamp (Unix epoch)
    uint32_t checksum;           // CRC32 checksum of entire file (excluding this field)
    
    uint32_t entry_point_rva;    // Relative Virtual Address of entry point (0 if none)
    uint32_t section_table_offset; // Offset to section table
    uint16_t section_count;      // Number of sections
    uint16_t reserved1;
    
    uint32_t symbol_table_offset;  // Offset to symbol table
    uint32_t symbol_count;         // Number of symbols
    uint32_t string_table_offset;  // Offset to string table
    uint32_t string_table_size;    // Size of string table
    
    uint32_t reloc_table_offset;   // Offset to relocation table
    uint32_t reloc_count;          // Number of relocations
    uint32_t import_table_offset;  // Offset to import table
    uint32_t import_count;         // Number of imports
    
    uint32_t export_table_offset;  // Offset to export table
    uint32_t export_count;         // Number of exports
    
    uint8_t  reserved2[8];         // Reserved for future use
} TccModHeader;
```

**Flags 定义：**
- `0x00000001` - `TCCMOD_FLAG_THUMB2`: Thumb-2 code (must be set for this project)
- `0x00000002` - `TCCMOD_FLAG_HARDFP`: Hard float ABI
- `0x00000004` - `TCCMOD_FLAG_PIC`: Position Independent Code
- `0x00000008` - `TCCMOD_FLAG_DEBUG`: Contains debug information

### 2. Section Table Entry (32 bytes)

```c
typedef struct {
    uint32_t name_offset;        // Offset in string table for section name
    uint32_t type;               // Section type (see below)
    uint32_t flags;              // Section flags (SHF_WRITE, SHF_ALLOC, SHF_EXECINSTR)
    uint32_t file_offset;        // Offset of section data in file
    uint32_t file_size;          // Size of section data in file
    uint32_t mem_size;           // Size in memory (may be larger for .bss)
    uint32_t alignment;          // Required alignment (power of 2)
    uint32_t reserved;           // Reserved
} TccModSection;
```

**Section Types:**
- `1` - `TCCMOD_SECT_TEXT`: Code section
- `2` - `TCCMOD_SECT_DATA`: Initialized data
- `3` - `TCCMOD_SECT_RODATA`: Read-only data
- `4` - `TCCMOD_SECT_BSS`: Uninitialized data
- `5` - `TCCMOD_SECT_CUSTOM`: Custom section

### 3. Symbol Table Entry (24 bytes)

```c
typedef struct {
    uint32_t name_offset;        // Offset in string table
    uint32_t value;              // Symbol value (RVA or absolute)
    uint32_t size;               // Symbol size
    uint16_t section_index;      // Section index (0 = undefined/external)
    uint8_t  type;               // Symbol type (see below)
    uint8_t  binding;            // Symbol binding (local/global/weak)
    uint32_t flags;              // Symbol flags
    uint32_t reserved;           // Reserved
} TccModSymbol;
```

**Symbol Types:**
- `0` - `TCCMOD_SYM_NOTYPE`: No type
- `1` - `TCCMOD_SYM_OBJECT`: Data object
- `2` - `TCCMOD_SYM_FUNC`: Function
- `3` - `TCCMOD_SYM_SECTION`: Section symbol
- `4` - `TCCMOD_SYM_FILE`: Source file name

**Symbol Binding:**
- `0` - `TCCMOD_BIND_LOCAL`: Local symbol
- `1` - `TCCMOD_BIND_GLOBAL`: Global symbol
- `2` - `TCCMOD_BIND_WEAK`: Weak symbol

### 4. Relocation Table Entry (16 bytes)

```c
typedef struct {
    uint32_t offset;             // Offset in section where relocation applies
    uint32_t symbol_index;       // Index into symbol table
    uint16_t type;               // Relocation type (ARM-specific)
    uint16_t section_index;      // Section index where relocation applies
    int32_t  addend;             // Addend for relocation calculation
} TccModReloc;
```

**ARM Thumb-2 Relocation Types (subset):**
- `1` - `R_ARM_ABS32`: Direct 32-bit
- `2` - `R_ARM_THM_CALL`: Thumb BL/BLX instruction
- `3` - `R_ARM_THM_JUMP24`: Thumb B instruction
- `4` - `R_ARM_THM_MOVW_ABS_NC`: Thumb MOVW instruction
- `5` - `R_ARM_THM_MOVT_ABS`: Thumb MOVT instruction

### 5. Import Table Entry (16 bytes)

```c
typedef struct {
    uint32_t name_offset;        // Offset in string table for symbol name
    uint32_t library_offset;     // Offset in string table for library name (optional)
    uint32_t hint;               // Import hint (ordinal or index)
    uint32_t flags;              // Import flags
} TccModImport;
```

### 6. Export Table Entry (12 bytes)

```c
typedef struct {
    uint32_t name_offset;        // Offset in string table for export name
    uint32_t address_rva;        // RVA of exported symbol
    uint32_t flags;              // Export flags
} TccModExport;
```

### 7. String Table

连续存储的以 null 结尾的字符串。第一个字节始终为 `\0`，表示空字符串（offset 0）。

## 加载流程

1. **验证头部**：检查 magic number 和版本兼容性
2. **校验完整性**：验证 CRC32 checksum
3. **分配内存**：
   - 为每个 section 分配适当的内存（遵循 W^X 策略）
   - .text: `VirtualAllocFromApp(PAGE_READWRITE)` → 稍后改为 `PAGE_EXECUTE_READ`
   - .data: `VirtualAllocFromApp(PAGE_READWRITE)`
   - .rodata: `VirtualAllocFromApp(PAGE_READWRITE)` → 稍后改为 `PAGE_READONLY`
   - .bss: `VirtualAllocFromApp(PAGE_READWRITE)` + `memset` 清零
4. **拷贝数据**：将 section 数据从文件拷贝到已分配的内存
5. **解析符号**：构建符号表，解析外部导入
6. **执行重定位**：根据重定位表修正地址引用
7. **设置权限**：
   - `VirtualProtectFromApp(.text, PAGE_EXECUTE_READ)`
   - `VirtualProtectFromApp(.rodata, PAGE_READONLY)`
   - `FlushInstructionCache()` 刷新指令缓存
8. **调用入口点**：跳转到 entry_point_rva（确保 LSB 设为 1 以保持 Thumb 状态）

## 与 TinyCC 的集成

### 扩展 TCC_OUTPUT_MEMORY

在 `tccrun.c` 中添加新的输出模式：

```c
#define TCC_OUTPUT_TCCMOD 6  /* .tccmod module file */
```

在 `tcc_relocate_ex` 函数中，当检测到 `TCC_OUTPUT_TCCMOD` 时，不执行内存分配和重定位，而是：

1. 收集所有 section 信息
2. 构建符号表和重定位表
3. 序列化为 `.tccmod` 格式
4. 写入文件

### 新增 API

```c
/* Serialize compiled code to .tccmod format */
LIBTCCAPI int tcc_output_tccmod(TCCState *s, const char *filename);

/* Load .tccmod file and return module handle */
LIBTCCAPI void* tccmod_load(const char *filename, TccModErrorFunc *error_func);

/* Get symbol address from loaded module */
LIBTCCAPI void* tccmod_get_symbol(void *module, const char *name);

/* Unload module and free memory */
LIBTCCAPI void tccmod_unload(void *module);
```

## 安全考虑

1. **代码签名**：可在 header 的 reserved 区域添加签名哈希
2. **边界检查**：加载器必须验证所有偏移量和大小，防止缓冲区溢出
3. **W^X 强制执行**：严格遵循 Windows 10 Mobile 的内存保护策略
4. **符号验证**：验证导入的符号确实存在于宿主进程中

## 示例工作流

```bash
# 编译 C 代码到 .tccmod 格式
arm-win32-tcc -o mymodule.tccmod mycode.c

# 在 UWP 应用中加载和执行
void *module = tccmod_load("mymodule.tccmod", error_handler);
int (*my_func)(int, int) = tccmod_get_symbol(module, "my_function");
int result = my_func(10, 20);
tccmod_unload(module);
```

## 未来扩展

- 压缩支持（LZMA/LZ4）
- 加密支持
- 增量更新机制
- 调试信息段（DWARF）
- 多架构支持（ARM64, x86）

# Project Chronos Collapse - Comprehensive Technical Guide & Specification

This guide merges all primary technical specifications, API references, structure layouts, and usage details for the `.tccmod` runtime system.

---

## Table of Contents

1. [Introduction](#1-introduction)
2. [Native Module Image Format Specification (.tccmod)](#2-native-module-image-format-specification-tccmod)
3. [Loader & Builder APIs](#3-loader--builder-apis)
4. [TinyCC Core Integration](#4-tinycc-core-integration)
5. [Project Directory & File Structure](#5-project-directory--file-structure)
6. [Debugging and Diagnostics](#6-debugging-and-diagnostics)

---

## 1. Introduction

`Project Chronos Collapse` implements a lightweight **Native Module Runtime** for the Windows 10 Mobile (ARM32, Lumia 950 XL) platform. Under Universal Windows Platform (UWP) app sandboxes, loading dynamic library files (`.dll`) from disk or download is restricted by OS-level Code Integrity checks. 

This project completely bypasses the dynamic PE loading mechanism of Windows by:
- Compiling code dynamically to a lightweight custom format (`.tccmod`).
- Hand-rolling a user-space module runtime loader to allocate code memory, perform symbol binding, apply Thumb-2 branch and data relocations, and transition page execution rights.
- Ensuring compliance with OS W^X (Write XOR Execute) memory policies via phased page protections (`VirtualAllocFromApp`/`VirtualProtectFromApp`).

---

## 2. Native Module Image Format Specification (.tccmod)

The `.tccmod` file structure layout is as follows:

```text
┌─────────────────────────────────────┐
│  File Header (80 bytes)             │
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

### 2.1 File Header (80 bytes)

Must be strictly aligned using `#pragma pack(1)`:

```c
typedef struct {
    uint32_t magic;                 /* Magic number: 0x4D434354 ("TCCM" in little-endian) */
    uint16_t version_major;         /* Format version major (1) */
    uint16_t version_minor;         /* Format version minor (0) */
    uint32_t flags;                 /* Module flags (e.g., TCCMOD_FLAG_THUMB2) */
    uint32_t timestamp;             /* Compilation timestamp (Unix epoch) */
    uint32_t checksum;              /* CRC32 checksum (excluding this field) */

    uint32_t entry_point_rva;       /* RVA of entry point (0 if none) */
    uint32_t section_table_offset;  /* Offset to section table */
    uint16_t section_count;         /* Number of sections */
    uint16_t reserved1;

    uint32_t symbol_table_offset;   /* Offset to symbol table */
    uint32_t symbol_count;          /* Number of symbols */
    uint32_t string_table_offset;   /* Offset to string table */
    uint32_t string_table_size;     /* Size of string table */

    uint32_t reloc_table_offset;    /* Offset to relocation table */
    uint32_t reloc_count;           /* Number of relocations */
    uint32_t import_table_offset;   /* Offset to import table */
    uint32_t import_count;          /* Number of imports */

    uint32_t export_table_offset;   /* Offset to export table */
    uint32_t export_count;          /* Number of exports */

    uint8_t  reserved2[8];          /* Reserved for future use */
} TccModHeader;
```

**Flags Definition:**
- `0x00000001` - `TCCMOD_FLAG_THUMB2`: ARM Thumb-2 code (mandatory for Windows 10 Mobile)
- `0x00000002` - `TCCMOD_FLAG_HARDFP`: Hard float ABI
- `0x00000004` - `TCCMOD_FLAG_PIC`: Position Independent Code
- `0x00000008` - `TCCMOD_FLAG_DEBUG`: Contains debug information

### 2.2 Section Table Entry (32 bytes)

```c
typedef struct {
    uint32_t name_offset;           /* Offset in string table for name */
    uint32_t type;                  /* Section type */
    uint32_t flags;                 /* Section flags */
    uint32_t file_offset;           /* Offset of data in file */
    uint32_t file_size;             /* Size of data in file */
    uint32_t mem_size;              /* Size in memory (e.g. larger for BSS) */
    uint32_t alignment;             /* Alignment requirements (power of 2) */
    uint32_t reserved;
} TccModSection;
```

**Section Types:**
- `1` - `TCCMOD_SECT_TEXT`: Code section
- `2` - `TCCMOD_SECT_DATA`: Initialized data
- `3` - `TCCMOD_SECT_RODATA`: Read-only data
- `4` - `TCCMOD_SECT_BSS`: Uninitialized data (not present in file, zero-filled in memory)
- `5` - `TCCMOD_SECT_CUSTOM`: Custom data section

**Section Flags:**
- `0x00000001` - `TCCMOD_SHF_WRITE`: Writable section
- `0x00000002` - `TCCMOD_SHF_ALLOC`: Allocates memory
- `0x00000004` - `TCCMOD_SHF_EXECINSTR`: Executable instructions

### 2.3 Symbol Table Entry (24 bytes)

```c
typedef struct {
    uint32_t name_offset;           /* Offset in string table */
    uint32_t value;                 /* Symbol value (RVA) */
    uint32_t size;                  /* Symbol size */
    uint16_t section_index;         /* Section index (0 = external import) */
    uint8_t  type;                  /* Symbol type */
    uint8_t  binding;               /* Symbol binding (local, global, weak) */
    uint32_t flags;                 /* Symbol flags */
    uint32_t reserved;
} TccModSymbol;
```

**Symbol Types:**
- `0` - `TCCMOD_SYM_NOTYPE`
- `1` - `TCCMOD_SYM_OBJECT`: Data object
- `2` - `TCCMOD_SYM_FUNC`: Executable function
- `3` - `TCCMOD_SYM_SECTION`
- `4` - `TCCMOD_SYM_FILE`

**Symbol Binding:**
- `0` - `TCCMOD_BIND_LOCAL`
- `1` - `TCCMOD_BIND_GLOBAL`
- `2` - `TCCMOD_BIND_WEAK`

### 2.4 Relocation Table Entry (16 bytes)

```c
typedef struct {
    uint32_t offset;                /* Offset in target section */
    uint32_t symbol_index;          /* Symbol index in Symbol Table */
    uint16_t type;                  /* Relocation type */
    uint16_t section_index;         /* Section index where relocation is applied */
    int32_t  addend;                /* Addend value */
} TccModReloc;
```

**ARM Thumb-2 Relocation Types:**
- `0` - `R_ARM_NONE`
- `2` - `R_ARM_ABS32`: Direct 32-bit relocation
- `10` - `R_ARM_THM_CALL`: Thumb BL/BLX branch call
- `30` - `R_ARM_THM_JUMP24`: Thumb B instruction jump
- `47` - `R_ARM_THM_MOVW_ABS_NC`: Thumb MOVW instruction NC immediate
- `48` - `R_ARM_THM_MOVT_ABS`: Thumb MOVT instruction MSB immediate

---

## 3. Loader & Builder APIs

### 3.1 Loader API (`tccmod_loader.c`)

```c
/* Opaque handle pointing to loaded runtime module */
typedef struct TccModRuntime TccModRuntime;

/* Error reporting callback */
typedef void (*TccModErrorFunc)(void *opaque, const char *msg);

/* Load a .tccmod from disk */
TccModRuntime* tccmod_load(const char *filename, TccModErrorFunc error_func, void *opaque);

/* Load a .tccmod image from memory buffer */
TccModRuntime* tccmod_load_memory(const void *buffer, size_t size, TccModErrorFunc error_func, void *opaque);

/* Query address of an exported symbol */
void* tccmod_get_symbol(TccModRuntime *mod, const char *name);

/* Register a host symbol so that the module can resolve external imports */
int tccmod_add_symbol(TccModRuntime *mod, const char *name, const void *addr);

/* Query the entry point of the module (if defined) */
void* tccmod_get_entry_point(TccModRuntime *mod);

/* Unload the module and release memory pages */
void tccmod_unload(TccModRuntime *mod);
```

### 3.2 Builder API (`tccmod_builder.c`)

```c
/* Serialize compiled code state to .tccmod file */
LIBTCCAPI int tcc_output_tccmod(TCCState *s, const char *filename);
```

---

## 4. TinyCC Core Integration

The `.tccmod` builder integrates straight into TinyCC's compilation pipeline via the output router.

### 4.1 Output Configuration
In `libtcc.h`, we define the output format type:
```c
#define TCC_OUTPUT_TCCMOD 6
```

### 4.2 Output Route Redirection
In `tccelf.c` under `tcc_output_file()`, compilation is intercepted:
```c
if (s->output_type == TCC_OUTPUT_TCCMOD)
    return tcc_output_tccmod(s, filename);
```

### 4.3 Windows Build Configuration
The MSVC Makefile `/tinycc/Makefile` includes `tccmod_builder.c` and `tccmod.h` into `CORE_FILES`. Under Windows `ONE_SOURCE` builds, the builder compilation requires `#include "tccmod_builder.c"` inside `tinycc/libtcc.c`.

---

## 5. Project Directory & File Structure

```text
Project-Chronos-Collapse/
├── README.md                      Project Overview & Quick Start
├── docs/                          Unified Project Documentation
│   ├── CHRONOS_COLLAPSE_GUIDE.md  Comprehensive Technical Specification & Guide (This File)
│   ├── DEVICE_DEPLOYMENT_GUIDE.md Lumia 950 XL Connection & File Deployment Guide
│   ├── PROJECT_HISTORY_AND_STATUS.md Project progress, milestones, and session logs
│   └── archive/                   Unconsolidated historical documentation files
├── tinycc/                        Tiny C Compiler Engine
│   ├── tccmod.h                   .tccmod definitions and structs
│   ├── tccmod_loader.c            Runtime module loader
│   ├── tccmod_builder.c           TCC serialization backend
│   ├── libtcc.h / tccelf.c        TinyCC modified core files
│   └── win32/                     Win32/ARM builds and scripts
│       ├── build_test.bat         Visual Studio unit-test build batch
│       ├── build-tcc.bat          TCC compiler build script
│       └── build/                 Testing build output directory
```

---

## 6. Debugging and Diagnostics

### 6.1 Layout Validation
Run `check_layout.exe` generated in the build directory to ensure your compiler outputs headers without unwanted padding. It must output:
```text
Total size: 80 bytes
Expected total: 80 bytes
OK: Structure size is correct.
```

### 6.2 Target LSB Alignment on ARM32 (Thumb-2)
When executing entry points or resolved functions on ARM32 Windows NT, you **must** set the function address's least significant bit (LSB) to `1`. Calling an even function address triggers an illegal CPU instruction fault since Windows only executes Thumb-2 code. The loader automatically performs this correction in `tccmod_get_symbol()` and `tccmod_get_entry_point()`.

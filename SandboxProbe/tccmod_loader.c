/*
 * TCC Native Module (.tccmod) Runtime Loader
 *
 * This file implements the runtime loader for .tccmod files on Windows ARM.
 * It handles memory allocation, relocation, and execution setup.
 *
 * Copyright (c) 2026 Project Chronos Collapse
 */

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#ifndef _CRT_NONSTDC_NO_DEPRECATE
#define _CRT_NONSTDC_NO_DEPRECATE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#endif

#include "tccmod.h"

/* Runtime module structure */
struct TccModRuntime {
    TccModHeader header;

    /* Loaded sections */
    void *text_base;
    size_t text_size;
    size_t text_alloc_size;  /* Actual allocated size (includes trampoline) */
    void *data_base;
    size_t data_size;
    void *rodata_base;
    size_t rodata_size;
    void *bss_base;
    size_t bss_size;

    /* Tables */
    TccModSection *sections;
    TccModSymbol *symbols;
    TccModReloc *relocs;
    TccModImport *imports;
    TccModExport *exports;
    char *string_table;

    /* Host symbol table for imports */
    struct {
        char *name;
        void *addr;
    } *host_symbols;
    int host_symbol_count;
    int host_symbol_capacity;

    /* Trampoline pool for long-range calls */
    void *trampoline_base;
    size_t trampoline_size;
    size_t trampoline_used;

    /* Linking state */
    int linked;

    /* Error handling */
    TccModErrorFunc error_func;
    void *error_opaque;
};

/* ===== Utility Functions ===== */

static void report_error(TccModRuntime *mod, const char *msg) {
    if (mod && mod->error_func)
        mod->error_func(mod->error_opaque, msg);
    else
        fprintf(stderr, "tccmod error: %s\n", msg);
}

/* Simple CRC32 implementation */
uint32_t tccmod_crc32(const void *data, size_t len) {
    const uint8_t *buf = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFF;
    size_t i, j;

    for (i = 0; i < len; i++) {
        crc ^= buf[i];
        for (j = 0; j < 8; j++) {
            if (crc & 1)
                crc = (crc >> 1) ^ 0xEDB88320;
            else
                crc = crc >> 1;
        }
    }
    return ~crc;
}

/* Verify module header */
int tccmod_verify_header(const TccModHeader *hdr) {
    if (hdr->magic != TCCMOD_MAGIC)
        return -1;
    if (hdr->version_major != TCCMOD_VERSION_MAJOR)
        return -1;
    /* Minor version differences are acceptable */
    return 0;
}

/* ===== Memory Allocation ===== */

#ifdef _WIN32
#include <winapifamily.h>

/* Use UWP-compliant APIs when building for Windows Store/Universal apps */
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    #define TCCMOD_ALLOC(size)                  VirtualAllocFromApp(NULL, (size), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
    #define TCCMOD_PROTECT(addr, sz, prot, old) VirtualProtectFromApp((addr), (sz), (prot), (old))
#else
    #define TCCMOD_ALLOC(size)                  VirtualAlloc(NULL, (size), MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE)
    #define TCCMOD_PROTECT(addr, sz, prot, old) VirtualProtect((addr), (sz), (prot), (old))
#endif
#endif

static void* alloc_executable_memory(size_t size) {
#ifdef _WIN32
    /* On Windows: allocate RW first, will change to RX later */
    return TCCMOD_ALLOC(size);
#else
    return mmap(NULL, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
}

/* Try to allocate memory near a target address (within ±16MB for ARM branches) */
static void* alloc_near_address(void* target, size_t size, size_t max_distance) {
#ifdef _WIN32
#if defined(WINAPI_FAMILY) && (WINAPI_FAMILY == WINAPI_FAMILY_APP)
    /* UWP doesn't allow specifying addresses for VirtualAllocFromApp */
    /* Just do regular allocation and hope for the best */
    return VirtualAllocFromApp(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
    /* Desktop Windows: try addresses around the target */
    uintptr_t target_addr = (uintptr_t)target;
    size_t step = 0x10000;  /* 64KB alignment */

    for (size_t offset = step; offset <= max_distance; offset += step) {
        /* Try below target */
        if (target_addr > offset) {
            uintptr_t try_addr = target_addr - offset;
            try_addr &= ~(step - 1);
            void* mem = VirtualAlloc((void*)try_addr, size,
                                     MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
            if (mem) {
                intptr_t distance = (intptr_t)mem - (intptr_t)target;
                if (distance >= -(intptr_t)max_distance && distance <= (intptr_t)max_distance) {
                    return mem;
                }
                VirtualFree(mem, 0, MEM_RELEASE);
            }
        }

        /* Try above target */
        uintptr_t try_addr = target_addr + offset;
        try_addr &= ~(step - 1);
        void* mem = VirtualAlloc((void*)try_addr, size,
                                 MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
        if (mem) {
            intptr_t distance = (intptr_t)mem - (intptr_t)target;
            if (distance >= -(intptr_t)max_distance && distance <= (intptr_t)max_distance) {
                return mem;
            }
            VirtualFree(mem, 0, MEM_RELEASE);
        }
    }
#endif
#endif
    /* Fallback: regular allocation */
    return alloc_executable_memory(size);
}

static void* alloc_data_memory(size_t size) {
#ifdef _WIN32
    return TCCMOD_ALLOC(size);
#else
    return mmap(NULL, size, PROT_READ | PROT_WRITE,
                MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
#endif
}

static int protect_memory(void *addr, size_t size, int executable, int writable) {
#ifdef _WIN32
    DWORD old_protect;
    DWORD new_protect;

    if (executable && !writable)
        new_protect = PAGE_EXECUTE_READ;
    else if (!executable && !writable)
        new_protect = PAGE_READONLY;
    else if (!executable && writable)
        new_protect = PAGE_READWRITE;
    else
        return -1; /* W^X violation */

    if (!TCCMOD_PROTECT(addr, size, new_protect, &old_protect))
        return -1;

    /* Flush instruction cache for executable memory */
    if (executable)
        FlushInstructionCache(GetCurrentProcess(), addr, size);

    return 0;
#else
    int prot = PROT_READ;
    if (executable) prot |= PROT_EXEC;
    if (writable) prot |= PROT_WRITE;
    return mprotect(addr, size, prot);
#endif
}

static void free_memory(void *addr, size_t size) {
#ifdef _WIN32
    if (addr)
        VirtualFree(addr, 0, MEM_RELEASE);
#else
    if (addr)
        munmap(addr, size);
#endif
}

/* ===== Symbol Resolution ===== */

int tccmod_add_symbol(TccModRuntime *mod, const char *name, const void *addr) {
    if (!mod || !name || !addr)
        return -1;

    /* Expand capacity if needed */
    if (mod->host_symbol_count >= mod->host_symbol_capacity) {
        int new_cap = mod->host_symbol_capacity ? mod->host_symbol_capacity * 2 : 16;
        void *new_syms = realloc(mod->host_symbols, new_cap * sizeof(mod->host_symbols[0]));
        if (!new_syms)
            return -1;
        mod->host_symbols = new_syms;
        mod->host_symbol_capacity = new_cap;
    }

    /* Add symbol */
    mod->host_symbols[mod->host_symbol_count].name = strdup(name);
    mod->host_symbols[mod->host_symbol_count].addr = (void*)addr;
    mod->host_symbol_count++;

    return 0;
}

static void* resolve_import(TccModRuntime *mod, const char *name) {
    int i;

    /* Search host symbol table */
    for (i = 0; i < mod->host_symbol_count; i++) {
        if (strcmp(mod->host_symbols[i].name, name) == 0)
            return mod->host_symbols[i].addr;
    }

    /* Not found */
    return NULL;
}

static void* get_section_base(TccModRuntime *mod, uint16_t section_index) {
    TccModSection *sect;

    if (section_index == 0 || section_index > mod->header.section_count)
        return NULL;

    sect = &mod->sections[section_index - 1];
    switch (sect->type) {
        case TCCMOD_SECT_TEXT:   return mod->text_base;
        case TCCMOD_SECT_DATA:   return mod->data_base;
        case TCCMOD_SECT_RODATA: return mod->rodata_base;
        case TCCMOD_SECT_BSS:    return mod->bss_base;
        default:                 return NULL;
    }
}

/* ===== Trampoline Support ===== */

/*
 * Create a trampoline for long-range calls on ARM Thumb-2
 * Trampoline code (8 bytes):
 *   ldr pc, [pc, #0]   ; Load target address into PC (PC+4 when executing this instruction)
 *   .word target_addr  ; 32-bit target address
 *
 * Note: In Thumb-2 mode, PC reads as (current instruction address + 4)
 * So "ldr pc, [pc, #0]" reads from address (trampoline + 4), which is where we store target_addr
 */
static void* create_trampoline(TccModRuntime *mod, void *target) {
    if (!mod->trampoline_base) {
        return NULL;
    }

    /* Check if we have space */
    if (mod->trampoline_used + 8 > mod->trampoline_size) {
        report_error(mod, "Trampoline pool exhausted");
        return NULL;
    }

    /* Get trampoline slot - must be 4-byte aligned */
    uint8_t *trampoline = (uint8_t*)mod->trampoline_base + mod->trampoline_used;

    /* Ensure 4-byte alignment for LDR instruction */
    if ((uintptr_t)trampoline & 3) {
        /* Round up to next 4-byte boundary */
        size_t padding = 4 - ((uintptr_t)trampoline & 3);
        trampoline += padding;
        mod->trampoline_used += padding;
    }

    mod->trampoline_used += 8;

    /* Write trampoline code (Thumb-2)
     * Encoding: ldr.w pc, [pc, #0]
     * This is a 32-bit Thumb-2 instruction split into two 16-bit halves
     */
    uint16_t *code = (uint16_t*)trampoline;
    code[0] = 0xF8DF;  /* ldr.w pc, [pc, #0] - first half */
    code[1] = 0xF000;  /* ldr.w pc, [pc, #0] - second half */

    /* Store target address at offset +4 from trampoline start
     * When the ldr instruction executes, PC = trampoline + 4,
     * so [pc, #0] = [trampoline + 4, #0] = trampoline + 4
     */
    uint32_t *addr = (uint32_t*)(trampoline + 4);
    *addr = (uint32_t)(uintptr_t)target;

    /* Set LSB for Thumb mode */
    return (void*)((uintptr_t)trampoline | 1);
}

/* ===== Relocation ===== */

static int apply_relocation(TccModRuntime *mod, TccModReloc *reloc) {
    TccModSymbol *sym;
    void *section_base;
    void *target_addr;
    uint32_t symbol_value;
    int32_t addend = reloc->addend;

    section_base = get_section_base(mod, reloc->section_index);

    if (!section_base) {
        report_error(mod, "Invalid section index in relocation");
        return -1;
    }

    target_addr = (uint8_t*)section_base + reloc->offset;

    /* Get symbol value */
    if (reloc->symbol_index >= mod->header.symbol_count) {
        report_error(mod, "Invalid symbol index in relocation");
        return -1;
    }

    sym = &mod->symbols[reloc->symbol_index];

    /* Resolve symbol */
    if (sym->section_index == 0) {
        /* External symbol - resolve from imports */
        const char *sym_name = mod->string_table + sym->name_offset;
        void *import_addr = resolve_import(mod, sym_name);
        if (!import_addr) {
            char err[256];
            snprintf(err, sizeof(err), "Unresolved import: %s", sym_name);
            report_error(mod, err);
            return -1;
        }
        symbol_value = (uint32_t)(uintptr_t)import_addr;
    } else {
        /* Symbol values are offsets in their owning section, not process addresses. */
        void *symbol_base = get_section_base(mod, sym->section_index);
        if (!symbol_base) {
            report_error(mod, "Invalid section index in symbol");
            return -1;
        }
        symbol_value = (uint32_t)(uintptr_t)symbol_base + sym->value;
    }

    /* Apply relocation based on type */
    switch (reloc->type) {
        case R_ARM_ABS32: {
            /* Direct 32-bit relocation */
            uint32_t *ptr = (uint32_t*)target_addr;
            *ptr = symbol_value + addend;
            break;
        }

        case R_ARM_THM_CALL:
        case R_ARM_THM_JUMP24: {
            /* Thumb BL/B instruction */
            uint16_t *insn = (uint16_t*)target_addr;

            /* For branch calculations, clear LSB (Thumb-2 bit) from symbol value */
            uint32_t branch_target = symbol_value & ~1;
            int32_t offset = branch_target + addend - (uint32_t)(uintptr_t)target_addr - 4;

            /* Check range: +/-16MB */
            if (offset < -0x1000000 || offset > 0x0FFFFFF) {
                /* Out of range - try to create trampoline */
                /* Pass the original symbol_value (with LSB if present) to trampoline */
                void *trampoline = create_trampoline(mod, (void*)(uintptr_t)symbol_value);
                if (!trampoline) {
                    char err[256];
                    snprintf(err, sizeof(err),
                            "Failed to create trampoline for out-of-range branch (offset=0x%X, ~%d MB)",
                            offset, (int)(offset / (1024*1024)));
                    report_error(mod, err);
                    return -1;
                }

                /* Recalculate offset to trampoline (clear LSB for calculation) */
                uint32_t trampoline_addr = (uint32_t)(uintptr_t)trampoline & ~1;
                offset = trampoline_addr - (uint32_t)(uintptr_t)target_addr - 4;

                /* Verify trampoline is in range */
                if (offset < -0x1000000 || offset > 0x0FFFFFF) {
                    char err[256];
                    snprintf(err, sizeof(err),
                            "Trampoline still out of range (offset=0x%X, ~%d MB). "
                            "Module @ 0x%X, Trampoline @ 0x%X, Target @ 0x%X",
                            offset, (int)(offset / (1024*1024)),
                            (uint32_t)(uintptr_t)target_addr,
                            trampoline_addr,
                            symbol_value);
                    report_error(mod, err);
                    return -1;
                }
            }

            /* Encode Thumb-2 branch */
            uint32_t s = (offset >> 24) & 1;
            uint32_t i1 = (offset >> 23) & 1;
            uint32_t i2 = (offset >> 22) & 1;
            uint32_t imm10 = (offset >> 12) & 0x3FF;
            uint32_t imm11 = (offset >> 1) & 0x7FF;

            insn[0] = (insn[0] & 0xF800) | (s << 10) | imm10;
            insn[1] = (insn[1] & 0xD000) | ((~(i1 ^ s) & 1) << 13) |
                      ((~(i2 ^ s) & 1) << 11) | imm11;
            break;
        }

        case R_ARM_THM_MOVW_ABS_NC:
        case R_ARM_THM_MOVT_ABS: {
            /* Thumb MOVW/MOVT instructions */
            uint16_t *insn = (uint16_t*)target_addr;
            uint32_t value = symbol_value + addend;

            if (reloc->type == R_ARM_THM_MOVT_ABS)
                value >>= 16;
            value &= 0xFFFF;

            /* Encode MOVW/MOVT immediate */
            uint32_t imm4 = (value >> 12) & 0xF;
            uint32_t i = (value >> 11) & 1;
            uint32_t imm3 = (value >> 8) & 7;
            uint32_t imm8 = value & 0xFF;

            insn[0] = (insn[0] & 0xFBF0) | (i << 10) | imm4;
            insn[1] = (insn[1] & 0x8F00) | (imm3 << 12) | imm8;
            break;
        }

        default:
            report_error(mod, "Unsupported relocation type");
            return -1;
    }

    return 0;
}

/* ===== Module Loading ===== */

TccModRuntime* tccmod_load_memory(const void *buffer, size_t size,
                                   TccModErrorFunc error_func, void *opaque) {
    TccModRuntime *mod = NULL;
    const uint8_t *data = (const uint8_t *)buffer;
    uint32_t i;

    if (!buffer || size < sizeof(TccModHeader)) {
        if (error_func)
            error_func(opaque, "Invalid buffer");
        return NULL;
    }

    /* Allocate module structure */
    mod = (TccModRuntime*)calloc(1, sizeof(TccModRuntime));
    if (!mod) {
        if (error_func)
            error_func(opaque, "Out of memory");
        return NULL;
    }

    mod->error_func = error_func;
    mod->error_opaque = opaque;

    /* Copy and verify header */
    memcpy(&mod->header, data, sizeof(TccModHeader));
    if (tccmod_verify_header(&mod->header) < 0) {
        report_error(mod, "Invalid module header");
        goto error;
    }

    /* Verify checksum */
    uint32_t original_checksum = mod->header.checksum;
    if (original_checksum != 0) {
        uint32_t crc = 0xFFFFFFFF;
        const uint8_t *buf = (const uint8_t *)data;
        for (size_t i = 0; i < size; i++) {
            uint8_t b = buf[i];
            if (i >= 16 && i < 20) {
                b = 0;
            }
            crc ^= b;
            for (size_t j = 0; j < 8; j++) {
                if (crc & 1)
                    crc = (crc >> 1) ^ 0xEDB88320;
                else
                    crc = crc >> 1;
            }
        }
        uint32_t computed_checksum = ~crc;
        if (computed_checksum != original_checksum) {
            report_error(mod, "CRC32 checksum verification failed");
            goto error;
        }
    }

    /* Load section table */
    if (mod->header.section_count > 0) {
        size_t sect_size = mod->header.section_count * sizeof(TccModSection);
        mod->sections = (TccModSection*)malloc(sect_size);
        if (!mod->sections) {
            report_error(mod, "Out of memory");
            goto error;
        }
        memcpy(mod->sections, data + mod->header.section_table_offset, sect_size);
    }

    /* Load string table */
    if (mod->header.string_table_size > 0) {
        mod->string_table = (char*)malloc(mod->header.string_table_size);
        if (!mod->string_table) {
            report_error(mod, "Out of memory");
            goto error;
        }
        memcpy(mod->string_table, data + mod->header.string_table_offset,
               mod->header.string_table_size);
    }

    /* Load symbols */
    if (mod->header.symbol_count > 0) {
        size_t sym_size = mod->header.symbol_count * sizeof(TccModSymbol);
        mod->symbols = (TccModSymbol*)malloc(sym_size);
        if (!mod->symbols) {
            report_error(mod, "Out of memory");
            goto error;
        }
        memcpy(mod->symbols, data + mod->header.symbol_table_offset, sym_size);
    }

    /* Load relocations */
    if (mod->header.reloc_count > 0) {
        size_t reloc_size = mod->header.reloc_count * sizeof(TccModReloc);
        mod->relocs = (TccModReloc*)malloc(reloc_size);
        if (!mod->relocs) {
            report_error(mod, "Out of memory");
            goto error;
        }
        memcpy(mod->relocs, data + mod->header.reloc_table_offset, reloc_size);
    }

    /* Allocate memory for sections */
    for (i = 0; i < mod->header.section_count; i++) {
        TccModSection *sect = &mod->sections[i];
        void **base_ptr = NULL;
        size_t *size_ptr = NULL;

        switch (sect->type) {
            case TCCMOD_SECT_TEXT:
                base_ptr = &mod->text_base;
                size_ptr = &mod->text_size;
                break;
            case TCCMOD_SECT_DATA:
                base_ptr = &mod->data_base;
                size_ptr = &mod->data_size;
                break;
            case TCCMOD_SECT_RODATA:
                base_ptr = &mod->rodata_base;
                size_ptr = &mod->rodata_size;
                break;
            case TCCMOD_SECT_BSS:
                base_ptr = &mod->bss_base;
                size_ptr = &mod->bss_size;
                break;
        }

        if (base_ptr && *base_ptr == NULL && sect->mem_size > 0) {
            size_t alloc_size = sect->mem_size;

            /* For text section, add space for trampoline pool at the end */
            if (sect->type == TCCMOD_SECT_TEXT) {
                alloc_size = sect->mem_size + 2048;  /* Add 2KB for trampolines */
            }

            if (sect->type == TCCMOD_SECT_TEXT)
                *base_ptr = alloc_executable_memory(alloc_size);
            else
                *base_ptr = alloc_data_memory(alloc_size);

            if (!*base_ptr) {
                report_error(mod, "Failed to allocate memory for section");
                goto error;
            }

            *size_ptr = sect->mem_size;  /* Store original section size */

            /* Copy section data */
            if (sect->file_size > 0) {
                memcpy(*base_ptr, data + sect->file_offset, sect->file_size);
            }

            /* Zero-fill the rest */
            if (alloc_size > sect->file_size) {
                memset((uint8_t*)*base_ptr + sect->file_size, 0,
                       alloc_size - sect->file_size);
            }

            /* Set up trampoline pool immediately after text section */
            if (sect->type == TCCMOD_SECT_TEXT) {
                mod->trampoline_base = (uint8_t*)*base_ptr + sect->mem_size;
                mod->trampoline_size = 2048;
                mod->trampoline_used = 0;
                mod->text_alloc_size = alloc_size;  /* Store actual allocated size */
            }
        }
    }

    return mod;

error:
    tccmod_unload(mod);
    return NULL;
}

int tccmod_link(TccModRuntime *mod) {
    uint32_t i;

    if (!mod)
        return -1;

    /* Check if already linked */
    if (mod->linked) {
        return 0;  /* Already linked, nothing to do */
    }

    /* Apply relocations */
    for (i = 0; i < mod->header.reloc_count; i++) {
        if (apply_relocation(mod, &mod->relocs[i]) < 0)
            return -1;
    }

    /* Set memory protection (text_alloc_size includes trampoline pool) */
    if (mod->text_base) {
        size_t protect_size = mod->text_alloc_size > 0 ? mod->text_alloc_size : mod->text_size;
        if (protect_memory(mod->text_base, protect_size, 1, 0) < 0) {
            report_error(mod, "Failed to protect text section");
            return -1;
        }
    }

    if (mod->rodata_base && protect_memory(mod->rodata_base, mod->rodata_size, 0, 0) < 0) {
        report_error(mod, "Failed to protect rodata section");
        return -1;
    }

    /* Mark as linked */
    mod->linked = 1;

    return 0;
}

TccModRuntime* tccmod_load(const char *filename, TccModErrorFunc error_func, void *opaque) {
    FILE *fp;
    size_t size;
    void *buffer;
    TccModRuntime *mod;

    fp = fopen(filename, "rb");
    if (!fp) {
        if (error_func)
            error_func(opaque, "Failed to open file");
        return NULL;
    }

    /* Get file size */
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* Read entire file */
    buffer = malloc(size);
    if (!buffer) {
        fclose(fp);
        if (error_func)
            error_func(opaque, "Out of memory");
        return NULL;
    }

    if (fread(buffer, 1, size, fp) != size) {
        free(buffer);
        fclose(fp);
        if (error_func)
            error_func(opaque, "Failed to read file");
        return NULL;
    }

    fclose(fp);

    /* Load from memory */
    mod = tccmod_load_memory(buffer, size, error_func, opaque);
    free(buffer);

    return mod;
}

void* tccmod_get_symbol(TccModRuntime *mod, const char *name) {
    uint32_t i;

    if (!mod || !name)
        return NULL;

    /* Search symbol table */
    for (i = 0; i < mod->header.symbol_count; i++) {
        TccModSymbol *sym = &mod->symbols[i];
        const char *sym_name = mod->string_table + sym->name_offset;

        if (strcmp(sym_name, name) == 0 && sym->binding == TCCMOD_BIND_GLOBAL) {
            /* Return address in appropriate section */
            void *base = NULL;

            if (sym->section_index > 0 && sym->section_index <= mod->header.section_count) {
                TccModSection *sect = &mod->sections[sym->section_index - 1];
                switch (sect->type) {
                    case TCCMOD_SECT_TEXT:   base = mod->text_base; break;
                    case TCCMOD_SECT_DATA:   base = mod->data_base; break;
                    case TCCMOD_SECT_RODATA: base = mod->rodata_base; break;
                    case TCCMOD_SECT_BSS:    base = mod->bss_base; break;
                }
            }

            if (base) {
                void *addr = (uint8_t*)base + sym->value;

                /* For ARM Thumb-2 functions, set LSB to 1 */
                if (sym->type == TCCMOD_SYM_FUNC && (mod->header.flags & TCCMOD_FLAG_THUMB2))
                    addr = (void*)((uintptr_t)addr | 1);

                return addr;
            }
        }
    }

    return NULL;
}

void* tccmod_get_entry_point(TccModRuntime *mod) {
    if (!mod || mod->header.entry_point_rva == 0)
        return NULL;

    void *entry = (uint8_t*)mod->text_base + mod->header.entry_point_rva;

    /* Set LSB for Thumb-2 */
    if (mod->header.flags & TCCMOD_FLAG_THUMB2)
        entry = (void*)((uintptr_t)entry | 1);

    return entry;
}

void tccmod_unload(TccModRuntime *mod) {
    int i;

    if (!mod)
        return;

    /* Free memory sections (use actual allocated size for text) */
    if (mod->text_base) {
        size_t free_size = mod->text_alloc_size > 0 ? mod->text_alloc_size : mod->text_size;
        free_memory(mod->text_base, free_size);
    }
    free_memory(mod->data_base, mod->data_size);
    free_memory(mod->rodata_base, mod->rodata_size);
    free_memory(mod->bss_base, mod->bss_size);

    /* Free tables */
    free(mod->sections);
    free(mod->symbols);
    free(mod->relocs);
    free(mod->imports);
    free(mod->exports);
    free(mod->string_table);

    /* Free host symbols */
    for (i = 0; i < mod->host_symbol_count; i++)
        free(mod->host_symbols[i].name);
    free(mod->host_symbols);

    free(mod);
}

void tccmod_list_exports(TccModRuntime *mod, void *ctx,
                         void (*callback)(void *ctx, const char *name, void *addr)) {
    uint32_t i;

    if (!mod || !callback)
        return;

    for (i = 0; i < mod->header.symbol_count; i++) {
        TccModSymbol *sym = &mod->symbols[i];
        if (sym->binding == TCCMOD_BIND_GLOBAL) {
            const char *name = mod->string_table + sym->name_offset;
            void *addr = tccmod_get_symbol(mod, name);
            if (addr)
                callback(ctx, name, addr);
        }
    }
}

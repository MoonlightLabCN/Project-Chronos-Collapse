/*
 * TCC Native Module (.tccmod) Format Definitions
 *
 * This header defines the binary format for serialized TinyCC compiled modules.
 * These modules can be loaded and executed at runtime without PE loader dependency.
 *
 * Copyright (c) 2026 Project Chronos Collapse
 */

#ifndef TCCMOD_H
#define TCCMOD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Magic number: "TCCM" in little-endian */
#define TCCMOD_MAGIC 0x4D434354

/* Current format version */
#define TCCMOD_VERSION_MAJOR 1
#define TCCMOD_VERSION_MINOR 0

/* Module flags */
#define TCCMOD_FLAG_THUMB2      0x00000001  /* ARM Thumb-2 code */
#define TCCMOD_FLAG_HARDFP      0x00000002  /* Hard float ABI */
#define TCCMOD_FLAG_PIC         0x00000004  /* Position Independent Code */
#define TCCMOD_FLAG_DEBUG       0x00000008  /* Contains debug info */

/* Section types */
#define TCCMOD_SECT_TEXT        1   /* Code section */
#define TCCMOD_SECT_DATA        2   /* Initialized data */
#define TCCMOD_SECT_RODATA      3   /* Read-only data */
#define TCCMOD_SECT_BSS         4   /* Uninitialized data */
#define TCCMOD_SECT_CUSTOM      5   /* Custom section */

/* Section flags (compatible with ELF SHF_* flags) */
#define TCCMOD_SHF_WRITE        0x00000001
#define TCCMOD_SHF_ALLOC        0x00000002
#define TCCMOD_SHF_EXECINSTR    0x00000004

/* Symbol types */
#define TCCMOD_SYM_NOTYPE       0   /* No type */
#define TCCMOD_SYM_OBJECT       1   /* Data object */
#define TCCMOD_SYM_FUNC         2   /* Function */
#define TCCMOD_SYM_SECTION      3   /* Section symbol */
#define TCCMOD_SYM_FILE         4   /* Source file name */

/* Symbol binding */
#define TCCMOD_BIND_LOCAL       0   /* Local symbol */
#define TCCMOD_BIND_GLOBAL      1   /* Global symbol */
#define TCCMOD_BIND_WEAK        2   /* Weak symbol */

/* ARM Thumb-2 relocation types (subset of standard ARM relocations) */
#define R_ARM_NONE              0
#define R_ARM_ABS32             2   /* Direct 32-bit */
#define R_ARM_THM_CALL          10  /* Thumb BL/BLX */
#define R_ARM_THM_JUMP24        30  /* Thumb B instruction */
#define R_ARM_THM_MOVW_ABS_NC   47  /* Thumb MOVW */
#define R_ARM_THM_MOVT_ABS      48  /* Thumb MOVT */

/* ===== Data Structures ===== */

/* Ensure structures are tightly packed without padding */
#pragma pack(push, 1)

/* File Header (80 bytes) */
typedef struct {
    uint32_t magic;                 /* Magic number: TCCMOD_MAGIC */
    uint16_t version_major;         /* Format version major */
    uint16_t version_minor;         /* Format version minor */
    uint32_t flags;                 /* Module flags */
    uint32_t timestamp;             /* Compilation timestamp */
    uint32_t checksum;              /* CRC32 of file (excluding this field) */

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

/* Section Table Entry (32 bytes) */
typedef struct {
    uint32_t name_offset;           /* Offset in string table */
    uint32_t type;                  /* Section type */
    uint32_t flags;                 /* Section flags */
    uint32_t file_offset;           /* Offset in file */
    uint32_t file_size;             /* Size in file */
    uint32_t mem_size;              /* Size in memory */
    uint32_t alignment;             /* Required alignment */
    uint32_t reserved;
} TccModSection;

/* Symbol Table Entry (24 bytes) */
typedef struct {
    uint32_t name_offset;           /* Offset in string table */
    uint32_t value;                 /* Symbol value (RVA) */
    uint32_t size;                  /* Symbol size */
    uint16_t section_index;         /* Section index (0 = external) */
    uint8_t  type;                  /* Symbol type */
    uint8_t  binding;               /* Symbol binding */
    uint32_t flags;                 /* Symbol flags */
    uint32_t reserved;
} TccModSymbol;

/* Relocation Table Entry (16 bytes) */
typedef struct {
    uint32_t offset;                /* Offset in section */
    uint32_t symbol_index;          /* Symbol table index */
    uint16_t type;                  /* Relocation type */
    uint16_t section_index;         /* Section index */
    int32_t  addend;                /* Addend */
} TccModReloc;

/* Import Table Entry (16 bytes) */
typedef struct {
    uint32_t name_offset;           /* Symbol name in string table */
    uint32_t library_offset;        /* Library name (0 = host) */
    uint32_t hint;                  /* Import hint */
    uint32_t flags;                 /* Import flags */
} TccModImport;

/* Export Table Entry (12 bytes) */
typedef struct {
    uint32_t name_offset;           /* Export name in string table */
    uint32_t address_rva;           /* RVA of symbol */
    uint32_t flags;                 /* Export flags */
} TccModExport;

#pragma pack(pop)

/* ===== Runtime Loader API ===== */

/* Opaque module handle */
typedef struct TccModRuntime TccModRuntime;

/* Error callback function type */
typedef void (*TccModErrorFunc)(void *opaque, const char *msg);

/* Load a .tccmod file into memory and prepare for execution */
TccModRuntime* tccmod_load(const char *filename, TccModErrorFunc error_func, void *opaque);

/* Load from memory buffer */
TccModRuntime* tccmod_load_memory(const void *buffer, size_t size, TccModErrorFunc error_func, void *opaque);

/* Get the address of an exported symbol */
void* tccmod_get_symbol(TccModRuntime *mod, const char *name);

/* Add a host symbol that the module can import */
int tccmod_add_symbol(TccModRuntime *mod, const char *name, const void *addr);

/* Unload module and free all resources */
void tccmod_unload(TccModRuntime *mod);

/* Get module entry point (if defined) */
void* tccmod_get_entry_point(TccModRuntime *mod);

/* List all exported symbols */
void tccmod_list_exports(TccModRuntime *mod, void *ctx,
                         void (*callback)(void *ctx, const char *name, void *addr));

/* ===== Serializer API (for TCC integration) ===== */

#ifdef TCC_INTERNAL

/* Context for building a .tccmod file */
typedef struct {
    TCCState *s;                    /* TCC state */
    FILE *fp;                       /* Output file */
    TccModHeader header;            /* File header */
    uint8_t *string_table;          /* String table buffer */
    uint32_t string_table_capacity;
    uint32_t string_table_used;
} TccModBuilder;

/* Initialize module builder */
int tccmod_builder_init(TccModBuilder *builder, TCCState *s, const char *filename);

/* Add a string to string table, return offset */
uint32_t tccmod_add_string(TccModBuilder *builder, const char *str);

/* Write sections to file */
int tccmod_write_sections(TccModBuilder *builder);

/* Write symbol table */
int tccmod_write_symbols(TccModBuilder *builder);

/* Write relocations */
int tccmod_write_relocs(TccModBuilder *builder);

/* Write imports/exports */
int tccmod_write_imports_exports(TccModBuilder *builder);

/* Finalize and close file */
int tccmod_builder_finish(TccModBuilder *builder);

#endif /* TCC_INTERNAL */

/* ===== Utility Functions ===== */

/* Calculate CRC32 checksum */
uint32_t tccmod_crc32(const void *data, size_t len);

/* Verify module header */
int tccmod_verify_header(const TccModHeader *hdr);

#ifdef __cplusplus
}
#endif

#endif /* TCCMOD_H */

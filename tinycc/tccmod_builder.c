/*
 * TCC Native Module (.tccmod) Builder/Serializer
 *
 * This file implements the serializer that converts TCC's compiled output
 * (sections, symbols, relocations) into the .tccmod file format.
 *
 * Copyright (c) 2026 Project Chronos Collapse
 */

#define TCC_INTERNAL
#include "tcc.h"
#include "tccmod.h"
#include <time.h>
#include <stddef.h>

/* String table management */
typedef struct {
    char *data;
    uint32_t size;
    uint32_t capacity;
} StringTable;

static void string_table_init(StringTable *st) {
    st->capacity = 4096;
    st->data = tcc_malloc(st->capacity);
    st->size = 1;  /* First byte is always '\0' */
    st->data[0] = '\0';
}

static void string_table_free(StringTable *st) {
    tcc_free(st->data);
}

static uint32_t string_table_add(StringTable *st, const char *str) {
    uint32_t len, offset;

    if (!str || !*str)
        return 0;  /* Empty string */

    /* Check if string already exists */
    for (offset = 1; offset < st->size; ) {
        if (strcmp(st->data + offset, str) == 0)
            return offset;
        offset += strlen(st->data + offset) + 1;
    }

    /* Add new string */
    len = strlen(str) + 1;
    if (st->size + len > st->capacity) {
        st->capacity = (st->size + len) * 2;
        st->data = tcc_realloc(st->data, st->capacity);
    }

    offset = st->size;
    memcpy(st->data + offset, str, len);
    st->size += len;

    return offset;
}

/* Builder context */
typedef struct {
    TCCState *s;
    FILE *fp;
    StringTable strings;

    /* Temporary buffers */
    TccModSection *sections;
    int section_count;

    TccModSymbol *symbols;
    int symbol_count;

    TccModReloc *relocs;
    int reloc_count;

    /* Section indices mapping */
    int text_sect_idx;
    int data_sect_idx;
    int rodata_sect_idx;
    int bss_sect_idx;
} TccModBuilder;

/* Initialize builder */
static int builder_init(TccModBuilder *b, TCCState *s1, const char *filename) {
    memset(b, 0, sizeof(*b));
    b->s = s1;

    b->fp = fopen(filename, "wb+");
    if (!b->fp) {
        tcc_error_noabort("could not open output file '%s'", filename);
        return -1;
    }

    string_table_init(&b->strings);

    return 0;
}

/* Cleanup builder */
static void builder_cleanup(TccModBuilder *b) {
    if (b->fp)
        fclose(b->fp);
    string_table_free(&b->strings);
    tcc_free(b->sections);
    tcc_free(b->symbols);
    tcc_free(b->relocs);
}

/* Collect sections from TCC state */
static int collect_sections(TccModBuilder *b) {
    TCCState *s1 = b->s;
    Section *sec;
    int i, count = 0;

    /* Count sections that should be included */
    for (i = 1; i < s1->nb_sections; i++) {
        sec = s1->sections[i];
        if (!(sec->sh_flags & SHF_ALLOC))
            continue;
        count++;
    }

    b->sections = tcc_mallocz(count * sizeof(TccModSection));
    b->section_count = 0;

    /* Convert TCC sections to tccmod format */
    for (i = 1; i < s1->nb_sections; i++) {
        sec = s1->sections[i];

        if (!(sec->sh_flags & SHF_ALLOC))
            continue;

        TccModSection *mod_sect = &b->sections[b->section_count];

        /* Add section name to string table */
        mod_sect->name_offset = string_table_add(&b->strings, sec->name);

        /* Determine section type */
        if (sec->sh_flags & SHF_EXECINSTR) {
            mod_sect->type = TCCMOD_SECT_TEXT;
            b->text_sect_idx = b->section_count + 1;
        } else if (sec->sh_type == SHT_NOBITS) {
            mod_sect->type = TCCMOD_SECT_BSS;
            b->bss_sect_idx = b->section_count + 1;
        } else if (sec->sh_flags & SHF_WRITE) {
            mod_sect->type = TCCMOD_SECT_DATA;
            b->data_sect_idx = b->section_count + 1;
        } else {
            mod_sect->type = TCCMOD_SECT_RODATA;
            b->rodata_sect_idx = b->section_count + 1;
        }

        /* Copy section flags */
        mod_sect->flags = 0;
        if (sec->sh_flags & SHF_WRITE)
            mod_sect->flags |= TCCMOD_SHF_WRITE;
        if (sec->sh_flags & SHF_ALLOC)
            mod_sect->flags |= TCCMOD_SHF_ALLOC;
        if (sec->sh_flags & SHF_EXECINSTR)
            mod_sect->flags |= TCCMOD_SHF_EXECINSTR;

        /* Section sizes */
        mod_sect->file_size = sec->data_offset;
        mod_sect->mem_size = sec->sh_size > sec->data_offset ? sec->sh_size : sec->data_offset;
        mod_sect->alignment = sec->sh_addralign;

        /* file_offset will be set when writing */

        b->section_count++;
    }

    return 0;
}

/* Collect symbols from TCC state */
static int collect_symbols(TccModBuilder *b) {
    TCCState *s1 = b->s;
    Section *symtab = s1->symtab;
    ElfW(Sym) *sym;
    int i, nb_syms;

    if (!symtab)
        return 0;

    nb_syms = symtab->data_offset / sizeof(ElfW(Sym));
    b->symbols = tcc_mallocz(nb_syms * sizeof(TccModSymbol));
    b->symbol_count = 0;

    for (i = 0; i < nb_syms; i++) {
        sym = (ElfW(Sym) *)symtab->data + i;

        TccModSymbol *mod_sym = &b->symbols[b->symbol_count];

        if (i == 0) {
            /* Dummy undefined symbol at index 0 */
            memset(mod_sym, 0, sizeof(*mod_sym));
            b->symbol_count++;
            continue;
        }

        /* Get symbol name */
        const char *name = (char *)symtab->link->data + sym->st_name;
        mod_sym->name_offset = string_table_add(&b->strings, name);

        /* Symbol value and size */
        mod_sym->value = sym->st_value;
        mod_sym->size = sym->st_size;

        /* Section index */
        mod_sym->section_index = sym->st_shndx;

        /* Symbol type */
        switch (ELFW(ST_TYPE)(sym->st_info)) {
            case STT_NOTYPE:  mod_sym->type = TCCMOD_SYM_NOTYPE; break;
            case STT_OBJECT:  mod_sym->type = TCCMOD_SYM_OBJECT; break;
            case STT_FUNC:    mod_sym->type = TCCMOD_SYM_FUNC; break;
            case STT_SECTION: mod_sym->type = TCCMOD_SYM_SECTION; break;
            case STT_FILE:    mod_sym->type = TCCMOD_SYM_FILE; break;
            default:          mod_sym->type = TCCMOD_SYM_NOTYPE; break;
        }

        /* Symbol binding */
        switch (ELFW(ST_BIND)(sym->st_info)) {
            case STB_LOCAL:  mod_sym->binding = TCCMOD_BIND_LOCAL; break;
            case STB_GLOBAL: mod_sym->binding = TCCMOD_BIND_GLOBAL; break;
            case STB_WEAK:   mod_sym->binding = TCCMOD_BIND_WEAK; break;
            default:         mod_sym->binding = TCCMOD_BIND_LOCAL; break;
        }

        b->symbol_count++;
    }

    return 0;
}

/* Collect relocations from TCC state */
static int is_alloc_reloc(TCCState *s1, Section *sec) {
    if (sec->sh_type != SHT_RELX)
        return 0;
    int target_idx = sec->sh_info;
    if (target_idx <= 0 || target_idx >= s1->nb_sections)
        return 0;
    return (s1->sections[target_idx]->sh_flags & SHF_ALLOC) != 0;
}

static int collect_relocations(TccModBuilder *b) {
    TCCState *s1 = b->s;
    Section *sec;
    int i, j, nb_relocs;

    /* Count total relocations */
    nb_relocs = 0;
    for (i = 1; i < s1->nb_sections; i++) {
        sec = s1->sections[i];
        if (is_alloc_reloc(s1, sec))
            nb_relocs += sec->data_offset / sizeof(ElfW_Rel);
    }

    if (nb_relocs == 0)
        return 0;

    b->relocs = tcc_mallocz(nb_relocs * sizeof(TccModReloc));
    b->reloc_count = 0;

    /* Collect relocations */
    for (i = 1; i < s1->nb_sections; i++) {
        sec = s1->sections[i];

        if (!is_alloc_reloc(s1, sec))
            continue;

        ElfW_Rel *rel = (ElfW_Rel *)sec->data;
        int n = sec->data_offset / sizeof(ElfW_Rel);

        for (j = 0; j < n; j++) {
            TccModReloc *mod_reloc = &b->relocs[b->reloc_count];

            mod_reloc->offset = rel[j].r_offset;
            mod_reloc->symbol_index = ELFW(R_SYM)(rel[j].r_info);
            mod_reloc->type = ELFW(R_TYPE)(rel[j].r_info);
            mod_reloc->section_index = sec->sh_info;

#ifdef TCC_TARGET_ARM
            /* Extract addend for ARM relocations */
            mod_reloc->addend = rel[j].r_addend;
#else
            mod_reloc->addend = 0;
#endif

            b->reloc_count++;
        }
    }

    return 0;
}

/* Write file header */
static int write_header(TccModBuilder *b, TccModHeader *hdr) {
    /* Fill in header */
    hdr->magic = TCCMOD_MAGIC;
    hdr->version_major = TCCMOD_VERSION_MAJOR;
    hdr->version_minor = TCCMOD_VERSION_MINOR;

    /* Set flags */
    hdr->flags = TCCMOD_FLAG_THUMB2;  /* ARM Thumb-2 code */
#ifdef TCC_ARM_HARDFLOAT
    hdr->flags |= TCCMOD_FLAG_HARDFP;
#endif

    hdr->timestamp = (uint32_t)time(NULL);
    hdr->checksum = 0;  /* Will be calculated later */

    hdr->section_count = b->section_count;
    hdr->symbol_count = b->symbol_count;
    hdr->reloc_count = b->reloc_count;
    hdr->string_table_size = b->strings.size;

    /* Entry point: look for _init or main */
    hdr->entry_point_rva = 0;  /* TODO: resolve entry point */

    /* Write header (will update offsets later) */
    if (fwrite(hdr, sizeof(TccModHeader), 1, b->fp) != 1)
        return -1;

    return 0;
}

/* Write sections and data */
static int write_sections(TccModBuilder *b) {
    TCCState *s1 = b->s;
    int i;

    for (i = 0; i < b->section_count; i++) {
        TccModSection *mod_sect = &b->sections[i];

        /* Find corresponding TCC section */
        Section *sec = NULL;
        const char *sect_name = b->strings.data + mod_sect->name_offset;

        for (int j = 1; j < s1->nb_sections; j++) {
            if (strcmp(s1->sections[j]->name, sect_name) == 0) {
                sec = s1->sections[j];
                break;
            }
        }

        if (!sec)
            continue;

        /* Set file offset */
        mod_sect->file_offset = ftell(b->fp);

        /* Write section data (skip BSS) */
        if (mod_sect->type != TCCMOD_SECT_BSS && sec->data) {
            if (fwrite(sec->data, 1, mod_sect->file_size, b->fp) != mod_sect->file_size)
                return -1;
        }
    }

    return 0;
}

/* Main API: output .tccmod file */
LIBTCCAPI int tcc_output_tccmod(TCCState *s, const char *filename) {
    TccModBuilder builder;
    TccModHeader header;
    long header_pos;
    int ret = -1;

    if (builder_init(&builder, s, filename) < 0)
        return -1;

    /* Collect data from TCC state */
    if (collect_sections(&builder) < 0)
        goto cleanup;

    if (collect_symbols(&builder) < 0)
        goto cleanup;

    if (collect_relocations(&builder) < 0)
        goto cleanup;

    /* Write header (with placeholder offsets) */
    memset(&header, 0, sizeof(header));
    header_pos = ftell(builder.fp);
    if (write_header(&builder, &header) < 0)
        goto cleanup;

    /* Write section table */
    header.section_table_offset = ftell(builder.fp);
    if (fwrite(builder.sections, sizeof(TccModSection), builder.section_count, builder.fp)
        != (size_t)builder.section_count)
        goto cleanup;

    /* Write section data */
    if (write_sections(&builder) < 0)
        goto cleanup;

    /* Write symbol table */
    header.symbol_table_offset = ftell(builder.fp);
    if (fwrite(builder.symbols, sizeof(TccModSymbol), builder.symbol_count, builder.fp)
        != (size_t)builder.symbol_count)
        goto cleanup;

    /* Write string table */
    header.string_table_offset = ftell(builder.fp);
    if (fwrite(builder.strings.data, 1, builder.strings.size, builder.fp)
        != builder.strings.size)
        goto cleanup;

    /* Write relocation table */
    header.reloc_table_offset = ftell(builder.fp);
    if (fwrite(builder.relocs, sizeof(TccModReloc), builder.reloc_count, builder.fp)
        != (size_t)builder.reloc_count)
        goto cleanup;

    /* Update section table with file offsets */
    fseek(builder.fp, header.section_table_offset, SEEK_SET);
    if (fwrite(builder.sections, sizeof(TccModSection), builder.section_count, builder.fp)
        != (size_t)builder.section_count)
        goto cleanup;

    /* Update header with correct offsets */
    fseek(builder.fp, header_pos, SEEK_SET);
    if (write_header(&builder, &header) < 0)
        goto cleanup;

    /* Calculate and update CRC32 checksum of the entire file */
    fseek(builder.fp, 0, SEEK_END);
    long file_size = ftell(builder.fp);
    if (file_size > 0) {
        unsigned char *file_buf = (unsigned char *)tcc_malloc(file_size);
        if (file_buf) {
            fseek(builder.fp, 0, SEEK_SET);
            if (fread(file_buf, 1, file_size, builder.fp) == (size_t)file_size) {
                /* Zero out the checksum field in the buffer for calculation */
                TccModHeader *temp_hdr = (TccModHeader *)file_buf;
                temp_hdr->checksum = 0;
                
                /* Calculate CRC32 using the same algorithm */
                uint32_t crc = 0xFFFFFFFF;
                for (long i = 0; i < file_size; i++) {
                    crc ^= file_buf[i];
                    for (int j = 0; j < 8; j++) {
                        if (crc & 1)
                            crc = (crc >> 1) ^ 0xEDB88320;
                        else
                            crc = crc >> 1;
                    }
                }
                uint32_t final_crc = ~crc;
                
                /* Write the final CRC back to the file */
                fseek(builder.fp, header_pos + offsetof(TccModHeader, checksum), SEEK_SET);
                fwrite(&final_crc, sizeof(final_crc), 1, builder.fp);
            }
            tcc_free(file_buf);
        }
    }

    ret = 0;

cleanup:
    builder_cleanup(&builder);
    return ret;
}

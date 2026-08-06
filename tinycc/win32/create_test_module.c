/*
 * Create a minimal valid .tccmod file for testing the loader
 * This generates a simple module with one function that returns 42
 */

#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "../tccmod.h"

/* ARM Thumb-2 code for: int test_function() { return 42; } */
static const uint8_t thumb2_code[] = {
    0x2A, 0x20,  /* movs r0, #42 */
    0x70, 0x47   /* bx lr */
};

int main(int argc, char **argv) {
    FILE *fp;
    TccModHeader hdr;
    TccModSection sections[1];
    TccModSymbol symbols[1];
    char string_table[64];
    uint32_t string_table_size;
    const char *output_file = "build/test_module.tccmod";

    if (argc > 1)
        output_file = argv[1];

    printf("Creating test .tccmod file: %s\n", output_file);

    /* Open output file */
    fp = fopen(output_file, "wb");
    if (!fp) {
        fprintf(stderr, "Failed to create file\n");
        return 1;
    }

    /* Build string table */
    string_table[0] = '\0';  /* Offset 0 = empty string */
    strcpy(string_table + 1, ".text");
    strcpy(string_table + 7, "test_function");
    string_table_size = 7 + strlen("test_function") + 1;

    /* Initialize header */
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = TCCMOD_MAGIC;
    hdr.version_major = TCCMOD_VERSION_MAJOR;
    hdr.version_minor = TCCMOD_VERSION_MINOR;
    hdr.flags = TCCMOD_FLAG_THUMB2;
    hdr.timestamp = (uint32_t)time(NULL);
    hdr.checksum = 0;  /* TODO: Calculate checksum */

    hdr.entry_point_rva = 0;  /* No entry point */
    hdr.section_count = 1;
    hdr.symbol_count = 1;
    hdr.reloc_count = 0;
    hdr.import_count = 0;
    hdr.export_count = 0;
    hdr.string_table_size = string_table_size;

    /* Calculate offsets */
    hdr.section_table_offset = sizeof(TccModHeader);
    hdr.symbol_table_offset = hdr.section_table_offset + sizeof(TccModSection) * hdr.section_count;
    hdr.string_table_offset = hdr.symbol_table_offset + sizeof(TccModSymbol) * hdr.symbol_count;
    hdr.reloc_table_offset = hdr.string_table_offset + hdr.string_table_size;

    /* Initialize .text section */
    memset(&sections[0], 0, sizeof(TccModSection));
    sections[0].name_offset = 1;  /* ".text" */
    sections[0].type = TCCMOD_SECT_TEXT;
    sections[0].flags = TCCMOD_SHF_ALLOC | TCCMOD_SHF_EXECINSTR;
    sections[0].file_offset = hdr.reloc_table_offset;  /* After all tables */
    sections[0].file_size = sizeof(thumb2_code);
    sections[0].mem_size = sizeof(thumb2_code);
    sections[0].alignment = 4;

    /* Initialize symbol for test_function */
    memset(&symbols[0], 0, sizeof(TccModSymbol));
    symbols[0].name_offset = 7;  /* "test_function" */
    symbols[0].value = 0;  /* Offset in .text section */
    symbols[0].size = sizeof(thumb2_code);
    symbols[0].section_index = 1;  /* First section */
    symbols[0].type = TCCMOD_SYM_FUNC;
    symbols[0].binding = TCCMOD_BIND_GLOBAL;

    /* Write header */
    fwrite(&hdr, sizeof(hdr), 1, fp);

    /* Write section table */
    fwrite(sections, sizeof(TccModSection), hdr.section_count, fp);

    /* Write symbol table */
    fwrite(symbols, sizeof(TccModSymbol), hdr.symbol_count, fp);

    /* Write string table */
    fwrite(string_table, 1, string_table_size, fp);

    /* Write code section data */
    fwrite(thumb2_code, 1, sizeof(thumb2_code), fp);

    fclose(fp);

    printf("Success! Created .tccmod file with:\n");
    printf("  - 1 section (.text, %zu bytes)\n", sizeof(thumb2_code));
    printf("  - 1 symbol (test_function)\n");
    printf("  - ARM Thumb-2 code: returns 42\n");
    printf("\nFile structure:\n");
    printf("  Header:         %zu bytes @ 0x00000000\n", sizeof(TccModHeader));
    printf("  Section table:  %zu bytes @ 0x%08X\n",
           sizeof(TccModSection) * hdr.section_count, hdr.section_table_offset);
    printf("  Symbol table:   %zu bytes @ 0x%08X\n",
           sizeof(TccModSymbol) * hdr.symbol_count, hdr.symbol_table_offset);
    printf("  String table:   %u bytes @ 0x%08X\n",
           hdr.string_table_size, hdr.string_table_offset);
    printf("  Code section:   %u bytes @ 0x%08X\n",
           sections[0].file_size, sections[0].file_offset);
    printf("  Total size:     %u bytes\n",
           sections[0].file_offset + sections[0].file_size);

    return 0;
}

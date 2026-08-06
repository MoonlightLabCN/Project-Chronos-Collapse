/*
 * Simple test to verify .tccmod file can be loaded
 * With detailed debug output
 */

#include <stdio.h>
#include <stdlib.h>
#include "../tccmod.h"

void error_handler(void *opaque, const char *msg) {
    fprintf(stderr, "[ERROR] %s\n", msg);
}

/* Fake host symbols for link verification */
int host_add(int a, int b) { return a + b; }
void host_print(const char *msg) {}

void print_export(void *ctx, const char *name, void *addr) {
    printf("   Exported symbol: '%s' at %p\n", name, addr);
}

int main(int argc, char **argv) {
    TccModRuntime *mod;
    FILE *fp;
    size_t filesize;

    printf("=== Simple TccMod Loader Test ===\n\n");

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <module.tccmod>\n", argv[0]);
        return 1;
    }

    printf("1. Opening file: %s\n", argv[1]);
    fp = fopen(argv[1], "rb");
    if (!fp) {
        fprintf(stderr, "   ERROR: Cannot open file\n");
        return 1;
    }

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    printf("   File size: %zu bytes\n", filesize);

    /* Read and verify header */
    TccModHeader hdr;
    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
        fprintf(stderr, "   ERROR: Cannot read header\n");
        fclose(fp);
        return 1;
    }

    printf("\n2. Verifying header:\n");
    printf("   Magic: 0x%08X ", hdr.magic);
    if (hdr.magic == TCCMOD_MAGIC) {
        printf("(OK)\n");
    } else {
        printf("(INVALID - expected 0x%08X)\n", TCCMOD_MAGIC);
        fclose(fp);
        return 1;
    }

    printf("   Version: %d.%d\n", hdr.version_major, hdr.version_minor);
    printf("   Flags: 0x%08X\n", hdr.flags);
    printf("   Section count: %d\n", hdr.section_count);
    printf("   Symbol count: %d\n", hdr.symbol_count);
    printf("   Reloc count: %d\n", hdr.reloc_count);

    /* Read and print symbol table */
    printf("\n[DEBUG] Raw Symbol Table from File:\n");
    TccModSymbol *symbols = malloc(hdr.symbol_count * sizeof(TccModSymbol));
    fseek(fp, hdr.symbol_table_offset, SEEK_SET);
    fread(symbols, sizeof(TccModSymbol), hdr.symbol_count, fp);

    char *strtab = malloc(hdr.string_table_size);
    fseek(fp, hdr.string_table_offset, SEEK_SET);
    fread(strtab, 1, hdr.string_table_size, fp);

    for (uint32_t i = 0; i < hdr.symbol_count; i++) {
        printf("  Symbol %u: '%s', value: 0x%08X, sect_idx: %d, binding: %d, type: %d\n",
               i, strtab + symbols[i].name_offset, symbols[i].value,
               symbols[i].section_index, symbols[i].binding, symbols[i].type);
    }
    /* Read and print section table */
    printf("\n[DEBUG] Raw Section Table from File:\n");
    TccModSection *sections = malloc(hdr.section_count * sizeof(TccModSection));
    fseek(fp, hdr.section_table_offset, SEEK_SET);
    fread(sections, sizeof(TccModSection), hdr.section_count, fp);
    for (uint32_t i = 0; i < hdr.section_count; i++) {
        printf("  Section %u: '%s', type: %d, flags: 0x%X, file_offset: %d, file_size: %d, mem_size: %d\n",
               i, strtab + sections[i].name_offset, sections[i].type, sections[i].flags,
               sections[i].file_offset, sections[i].file_size, sections[i].mem_size);
    }
    free(sections);

    free(symbols);
    free(strtab);

    fclose(fp);

    printf("\n3. Loading module...\n");
    mod = tccmod_load(argv[1], error_handler, NULL);
    if (!mod) {
        fprintf(stderr, "   ERROR: Failed to load module\n");
        return 1;
    }
    printf("   Module loaded successfully!\n");

    /* Register fake host symbols so we can link relocations */
    tccmod_add_symbol(mod, "host_add", host_add);
    tccmod_add_symbol(mod, "host_print", host_print);

    printf("\n3.5 Linking module...\n");
    if (tccmod_link(mod) < 0) {
        fprintf(stderr, "   ERROR: Failed to link module\n");
        tccmod_unload(mod);
        return 1;
    }
    printf("   Module linked successfully!\n");

    printf("\n4. Listing exported symbols...\n");
    tccmod_list_exports(mod, NULL, print_export);

    printf("\n5. Note: Cannot execute ARM code on x86_64\n");
    printf("   This test validates file format and loading only.\n");
    printf("   Actual execution requires ARM device (Lumia 950 XL).\n");

    printf("\n=== TEST PASSED ===\n");
    printf("The .tccmod file format is valid and loader works correctly!\n");

    tccmod_unload(mod);
    return 0;
}

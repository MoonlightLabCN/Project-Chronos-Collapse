/*
 * Debug tool to verify TccModHeader structure layout
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include "../tccmod.h"

#define PRINT_OFFSET(type, field) \
    printf("  %-30s offset: %2zu, size: %2zu\n", #field, offsetof(type, field), sizeof(((type*)0)->field))

int main() {
    printf("=== TccModHeader Structure Layout ===\n");
    printf("Total size: %zu bytes\n\n", sizeof(TccModHeader));

    PRINT_OFFSET(TccModHeader, magic);
    PRINT_OFFSET(TccModHeader, version_major);
    PRINT_OFFSET(TccModHeader, version_minor);
    PRINT_OFFSET(TccModHeader, flags);
    PRINT_OFFSET(TccModHeader, timestamp);
    PRINT_OFFSET(TccModHeader, checksum);
    PRINT_OFFSET(TccModHeader, entry_point_rva);
    PRINT_OFFSET(TccModHeader, section_table_offset);
    PRINT_OFFSET(TccModHeader, section_count);
    PRINT_OFFSET(TccModHeader, reserved1);
    PRINT_OFFSET(TccModHeader, symbol_table_offset);
    PRINT_OFFSET(TccModHeader, symbol_count);
    PRINT_OFFSET(TccModHeader, string_table_offset);
    PRINT_OFFSET(TccModHeader, string_table_size);
    PRINT_OFFSET(TccModHeader, reloc_table_offset);
    PRINT_OFFSET(TccModHeader, reloc_count);
    PRINT_OFFSET(TccModHeader, import_table_offset);
    PRINT_OFFSET(TccModHeader, import_count);
    PRINT_OFFSET(TccModHeader, export_table_offset);
    PRINT_OFFSET(TccModHeader, export_count);
    PRINT_OFFSET(TccModHeader, reserved2);

    printf("\nExpected total: 80 bytes\n");
    if (sizeof(TccModHeader) != 80) {
        printf("WARNING: Structure size mismatch! Got %zu bytes.\n", sizeof(TccModHeader));
        return 1;
    } else {
        printf("OK: Structure size is correct.\n");
    }

    return 0;
}

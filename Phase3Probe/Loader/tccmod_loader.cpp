/*
 * TccMod Loader - UWP Adaptation
 * Main changes:
 * - VirtualAlloc -> VirtualAllocFromApp
 * - VirtualProtect -> VirtualProtectFromApp
 * - File reading uses Windows::Storage API
 */

#include "tccmod_loader.h"
#include "tccmod.h"
#include <windows.h>
#include <vector>
#include <string>
#include <cstring>

struct TccModRuntime {
    TccModHeader header;
    void* code_mem;
    void* data_mem;
    size_t code_size;
    size_t data_size;
    std::vector<uint8_t> file_data;
    std::string error_msg;
};

static void* uwp_alloc_memory(size_t size, DWORD protect) {
    return VirtualAllocFromApp(
        NULL,
        size,
        MEM_COMMIT | MEM_RESERVE,
        protect
    );
}

static bool uwp_protect_memory(void* addr, size_t size, DWORD new_protect) {
    DWORD old_protect;
    return VirtualProtectFromApp(addr, size, new_protect, &old_protect) != 0;
}

TccModRuntime* tccmod_load_from_buffer(const uint8_t* buffer, size_t size, std::string& error) {
    const TccModSection* sections;
    const TccModSection* text_section;
    uint16_t i;

    error.clear();

    if (!buffer || size < sizeof(TccModHeader)) {
        error = "Invalid buffer or size too small";
        return nullptr;
    }

    TccModRuntime* mod = new TccModRuntime();
    mod->code_mem = nullptr;
    mod->data_mem = nullptr;
    mod->code_size = 0;
    mod->data_size = 0;
    mod->file_data.assign(buffer, buffer + size);

    memcpy(&mod->header, buffer, sizeof(TccModHeader));

    if (mod->header.magic != TCCMOD_MAGIC) {
        error = "Invalid magic number: 0x" + std::to_string(mod->header.magic);
        delete mod;
        return nullptr;
    }

    if (mod->header.version_major != 1 || mod->header.version_minor != 0) {
        error = "Unsupported version: " +
                std::to_string(mod->header.version_major) + "." +
                std::to_string(mod->header.version_minor);
        delete mod;
        return nullptr;
    }

    if (mod->header.section_count == 0) {
        error = "No sections found";
        delete mod;
        return nullptr;
    }

    sections = reinterpret_cast<const TccModSection*>(buffer + mod->header.section_table_offset);

    text_section = nullptr;
    for (i = 0; i < mod->header.section_count; i++) {
        if (sections[i].type == TCCMOD_SECT_TEXT) {
            text_section = &sections[i];
            break;
        }
    }

    if (!text_section) {
        error = "No .text section found";
        delete mod;
        return nullptr;
    }

    mod->code_size = text_section->mem_size;
    mod->code_mem = uwp_alloc_memory(mod->code_size, PAGE_READWRITE);

    if (!mod->code_mem) {
        error = "Failed to allocate code memory";
        delete mod;
        return nullptr;
    }

    memcpy(mod->code_mem, buffer + text_section->file_offset, text_section->file_size);

    if (!uwp_protect_memory(mod->code_mem, mod->code_size, PAGE_EXECUTE_READ)) {
        error = "Failed to set execute permission";
        VirtualFree(mod->code_mem, 0, MEM_RELEASE);
        delete mod;
        return nullptr;
    }

    FlushInstructionCache(GetCurrentProcess(), mod->code_mem, mod->code_size);

    return mod;
}

void* tccmod_get_symbol(TccModRuntime* mod, const char* name) {
    const uint8_t* buffer;
    const TccModSymbol* symbols;
    const char* string_table;
    uint32_t i;
    const char* sym_name;
    void* addr;

    if (!mod || !name) return nullptr;

    buffer = mod->file_data.data();
    symbols = reinterpret_cast<const TccModSymbol*>(buffer + mod->header.symbol_table_offset);
    string_table = reinterpret_cast<const char*>(buffer + mod->header.string_table_offset);

    for (i = 0; i < mod->header.symbol_count; i++) {
        sym_name = string_table + symbols[i].name_offset;

        if (strcmp(sym_name, name) == 0) {
            addr = static_cast<uint8_t*>(mod->code_mem) + symbols[i].value;

            if (symbols[i].type == TCCMOD_SYM_FUNC &&
                (mod->header.flags & TCCMOD_FLAG_THUMB2)) {
                addr = reinterpret_cast<void*>(
                    reinterpret_cast<uintptr_t>(addr) | 1
                );
            }

            return addr;
        }
    }

    return nullptr;
}

void tccmod_unload(TccModRuntime* mod) {
    if (!mod) return;

    if (mod->code_mem) {
        VirtualFree(mod->code_mem, 0, MEM_RELEASE);
    }

    if (mod->data_mem) {
        VirtualFree(mod->data_mem, 0, MEM_RELEASE);
    }

    delete mod;
}

const char* tccmod_get_error(TccModRuntime* mod) {
    if (!mod) return "Invalid module handle";
    return mod->error_msg.empty() ? nullptr : mod->error_msg.c_str();
}

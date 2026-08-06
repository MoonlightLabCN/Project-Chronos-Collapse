/*
 * TccMod Loader - UWP Header
 */

#pragma once

#include <stdint.h>
#include <string>

struct TccModRuntime;

#ifdef __cplusplus
extern "C" {
#endif

TccModRuntime* tccmod_load_from_buffer(
    const uint8_t* buffer,
    size_t size,
    std::string& error
);

void* tccmod_get_symbol(TccModRuntime* mod, const char* name);

void tccmod_unload(TccModRuntime* mod);

const char* tccmod_get_error(TccModRuntime* mod);

#ifdef __cplusplus
}
#endif

/*
 * Test program for .tccmod loader
 *
 * This program demonstrates loading a .tccmod module and executing functions from it.
 */

#include <stdio.h>
#include <stdlib.h>
#include "../tccmod.h"

void error_handler(void *opaque, const char *msg) {
    fprintf(stderr, "ERROR: %s\n", msg);
}

/* Host functions that the module can import */
int host_add(int a, int b) {
    return a + b;
}

void host_print(const char *msg) {
    printf("[HOST] %s\n", msg);
}

int main(int argc, char **argv) {
    TccModRuntime *mod;
    int (*test_func)(int, int);
    int result;

    printf("=== TccMod Loader Test ===\n\n");

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <module.tccmod>\n", argv[0]);
        return 1;
    }

    /* Load the module */
    printf("1. Loading module: %s\n", argv[1]);
    mod = tccmod_load(argv[1], error_handler, NULL);
    if (!mod) {
        fprintf(stderr, "Failed to load module\n");
        return 1;
    }
    printf("   Module loaded successfully!\n\n");

    /* Register host symbols */
    printf("2. Registering host symbols...\n");
    tccmod_add_symbol(mod, "host_add", host_add);
    tccmod_add_symbol(mod, "host_print", host_print);
    printf("   Host symbols registered.\n\n");

    /* Link the module */
    printf("3. Linking module (resolving relocations)...\n");
    if (tccmod_link(mod) < 0) {
        fprintf(stderr, "Failed to link module\n");
        tccmod_unload(mod);
        return 1;
    }
    printf("   Module linked successfully!\n\n");

    /* Get a function from the module */
    printf("4. Resolving exported function...\n");
    test_func = (int (*)(int, int))tccmod_get_symbol(mod, "test_function");
    if (!test_func) {
        fprintf(stderr, "Failed to find 'test_function' in module\n");
        tccmod_unload(mod);
        return 1;
    }
    printf("   'test_function' found at: %p\n\n", test_func);

    /* Execute the function */
    printf("5. Executing module function...\n");
    result = test_func(10, 32);
    printf("   test_function(10, 32) = %d (expected: 42)\n\n", result);

    if (result != 42) {
        fprintf(stderr, "ERROR: Function returned wrong value!\n");
        tccmod_unload(mod);
        return 1;
    }

    /* Check for entry point */
    printf("6. Checking entry point...\n");
    void (*entry)(void) = (void (*)(void))tccmod_get_entry_point(mod);
    if (entry) {
        printf("   Entry point found at: %p\n", entry);
        printf("   Calling entry point...\n");
        entry();
    } else {
        printf("   No entry point defined.\n");
    }

    printf("\n=== TEST PASSED ===\n");

    /* Cleanup */
    tccmod_unload(mod);

    return 0;
}

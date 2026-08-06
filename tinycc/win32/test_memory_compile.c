/*
 * Test TCC_OUTPUT_MEMORY mode for ARM Thumb-2 code generation
 * This verifies that TinyCC can compile C code directly to memory
 * and execute it - which is the foundation for our .tccmod runtime.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../libtcc.h"

/* Test code to compile at runtime */
const char *test_code =
    "int add(int a, int b) {\n"
    "    return a + b;\n"
    "}\n"
    "\n"
    "int factorial(int n) {\n"
    "    if (n <= 1) return 1;\n"
    "    return n * factorial(n - 1);\n"
    "}\n";

void error_func(void *opaque, const char *msg) {
    fprintf(stderr, "TCC Error: %s\n", msg);
}

int main(int argc, char **argv) {
    TCCState *s;
    int (*add_func)(int, int);
    int (*factorial_func)(int);
    int result;

    printf("=== TinyCC Memory Compilation Test ===\n");
    printf("Target: ARM Thumb-2 (Windows ARMNT)\n\n");

    /* Create TCC state */
    s = tcc_new();
    if (!s) {
        fprintf(stderr, "Could not create TCC state\n");
        return 1;
    }

    /* Set error handler */
    tcc_set_error_func(s, NULL, error_func);

    /* Set output type to memory */
    printf("1. Setting output type to TCC_OUTPUT_MEMORY...\n");
    if (tcc_set_output_type(s, TCC_OUTPUT_MEMORY) < 0) {
        fprintf(stderr, "Failed to set output type\n");
        return 1;
    }

    /* Compile the test code */
    printf("2. Compiling test code...\n");
    if (tcc_compile_string(s, test_code) < 0) {
        fprintf(stderr, "Compilation failed\n");
        return 1;
    }
    printf("   Compilation successful!\n");

    /* Relocate the code */
    printf("3. Relocating code in memory...\n");
    if (tcc_relocate(s) < 0) {
        fprintf(stderr, "Relocation failed\n");
        return 1;
    }
    printf("   Relocation successful!\n");

    /* Get function pointers */
    printf("4. Resolving symbols...\n");
    add_func = tcc_get_symbol(s, "add");
    if (!add_func) {
        fprintf(stderr, "Could not find 'add' function\n");
        return 1;
    }
    printf("   'add' function at: %p\n", add_func);

    factorial_func = tcc_get_symbol(s, "factorial");
    if (!factorial_func) {
        fprintf(stderr, "Could not find 'factorial' function\n");
        return 1;
    }
    printf("   'factorial' function at: %p\n", factorial_func);

    /* Execute the compiled code */
    printf("\n5. Executing compiled functions:\n");

    result = add_func(15, 27);
    printf("   add(15, 27) = %d (expected: 42)\n", result);
    if (result != 42) {
        fprintf(stderr, "ERROR: add() returned wrong value!\n");
        return 1;
    }

    result = factorial_func(5);
    printf("   factorial(5) = %d (expected: 120)\n", result);
    if (result != 120) {
        fprintf(stderr, "ERROR: factorial() returned wrong value!\n");
        return 1;
    }

    result = factorial_func(10);
    printf("   factorial(10) = %d (expected: 3628800)\n", result);
    if (result != 3628800) {
        fprintf(stderr, "ERROR: factorial() returned wrong value!\n");
        return 1;
    }

    printf("\n=== ALL TESTS PASSED ===\n");
    printf("TinyCC successfully compiled and executed Thumb-2 code in memory!\n");

    /* Cleanup */
    tcc_delete(s);

    return 0;
}

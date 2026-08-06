// test_compile_indirect.c
// Modified version that uses function pointers instead of direct calls
// to avoid ARM branch range limitations

// Function pointer declarations
extern int (*host_add_ptr)(int, int);
extern void (*host_print_ptr)(const char*);

// Helper macros for indirect calls
#define host_add(a, b) (host_add_ptr ? host_add_ptr(a, b) : 0)
#define host_print(msg) (host_print_ptr ? host_print_ptr(msg) : (void)0)

// Simple test function
int test_function() {
    return 42;
}

// Function that uses host callbacks
int calculate(int a, int b) {
    int sum = host_add(a, b);

    int result = sum * sum;
    return result;
}

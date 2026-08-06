// Test TCC compilation to .tccmod
#include <stdio.h>

int add(int a, int b) {
    return a + b;
}

int multiply(int x, int y) {
    return x * y;
}

int get_answer(void) {
    return 42;
}

const char* get_message(void) {
    return "Hello from TCC!";
}

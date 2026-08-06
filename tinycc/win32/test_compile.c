/*
 * Simple test program for .tccmod compilation
 * This will be compiled by TCC to .tccmod format
 */

// External functions that will be provided by the host
extern int host_add(int a, int b);
extern void host_print(const char *msg);

// Internal data
static int counter = 0;
const char *greeting = "Hello from compiled module!";

// Internal helper function
static int square(int x) {
    return x * x;
}

// Exported function 1: Simple arithmetic
int calculate(int a, int b) {
    counter++;
    int sum = host_add(a, b);
    int sq = square(sum);
    return sq;
}

// Exported function 2: Get counter value
int get_counter(void) {
    return counter;
}

// Exported function 3: Use host print
void greet(void) {
    host_print(greeting);
}

// Exported function 4: Multiple operations
int complex_calc(int x, int y, int z) {
    int a = host_add(x, y);
    int b = square(z);
    int c = host_add(a, b);
    counter += 10;
    return c;
}

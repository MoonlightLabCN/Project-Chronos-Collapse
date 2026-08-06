/*
 * Example module source code for .tccmod format testing
 *
 * This module will be compiled to .tccmod format and loaded by the test program.
 */

/* External symbols provided by host */
extern int host_add(int a, int b);
extern void host_print(const char *msg);

/* Module's own data */
static int internal_counter = 0;
const char *message = "Hello from TccMod!";

/* Private function */
static int internal_multiply(int a, int b) {
    return a * b;
}

/* Exported function */
int test_function(int x, int y) {
    internal_counter++;

    /* Use host function */
    int sum = host_add(x, y);

    /* Use internal function */
    int product = internal_multiply(internal_counter, 10);

    return sum;
}

/* Another exported function */
int get_counter(void) {
    return internal_counter;
}

/* Function that uses host print */
void greet(void) {
    host_print(message);
}

/* Entry point (if defined) */
void _init(void) {
    host_print("Module initialized!");
}

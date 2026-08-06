#include <stdio.h>
#include <string.h>

/* Test arithmetic, pointers, and loops */
int fib(int n) {
    if (n <= 1) return n;
    return fib(n-1) + fib(n-2);
}

int sum_array(int *arr, int n) {
    int s = 0;
    for (int i = 0; i < n; i++) s += arr[i];
    return s;
}

int main() {
    /* Basic arithmetic */
    int a = 123, b = 456;
    printf("add: %d\n", a + b);
    printf("mul: %d\n", a * b);
    printf("div: %d\n", b / a);
    printf("mod: %d\n", b % a);

    /* Fibonacci */
    printf("fib(10): %d\n", fib(10));

    /* Array / pointer */
    int arr[5] = {1, 2, 3, 4, 5};
    printf("sum: %d\n", sum_array(arr, 5));

    /* String */
    char buf[32];
    strcpy(buf, "ARMNT");
    printf("str: %s len=%d\n", buf, (int)strlen(buf));

    return 0;
}

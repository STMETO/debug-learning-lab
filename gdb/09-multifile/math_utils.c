#include "math_utils.h"

int gcd(int a, int b) {
    while (b != 0) {
        int temp = b;
        b = a % b;
        a = temp;
    }
    return a;
}

unsigned long long factorial(int n) {
    if (n <= 1) return 1;
    unsigned long long result = n * factorial(n - 1);
    return result;
}

unsigned long long power(int base, int exp) {
    unsigned long long result = 1;
    for (int i = 0; i < exp; i++) {
        result *= base;
    }
    return result;
}

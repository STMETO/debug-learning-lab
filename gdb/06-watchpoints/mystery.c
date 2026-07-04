#include <stdio.h>

// ── 一个"神秘"的程序 ──
// variable counter 在程序的各个角落被修改
// 用 watch 命令找出所有修改它的地方

int counter = 0;   // global variable → 观察目标

void phase_one() {
    counter += 10;    // 第 1 修改
}

void phase_two() {
    counter *= 3;      // 第 2 修改
}

void phase_three() {
    counter -= 4;      // 第 3 修改
}

void phase_four() {
    counter = counter * counter;  // 第 4 修改（平方）
}

void helper_a() {
    counter += 1;      // 第 5 修改
}

void helper_b() {
    counter += 2;      // 第 6 修改
}

void phase_five() {
    // 这一Phase连续调用两个函数修改 counter
    helper_a();
    helper_b();
}

int main() {
    printf("Initial counter = %d\n", counter);

    phase_one();
    printf("After phase_one:   counter = %d\n", counter);

    phase_two();
    printf("After phase_two:   counter = %d\n", counter);

    phase_three();
    printf("After phase_three: counter = %d\n", counter);

    phase_four();
    printf("After phase_four:  counter = %d\n", counter);

    phase_five();
    printf("After phase_five:  counter = %d\n", counter);

    printf("\nFinal counter = %d\n", counter);
    return 0;
}

#include <stdio.h>

// ── 一个"神秘"的程序 ──
// 变量 counter 在程序的各个角落被修改
// 用 watch 命令找出所有修改它的地方

int counter = 0;   // 全局变量 → 观察目标

void phase_one() {
    counter += 10;    // 第 1 次修改
}

void phase_two() {
    counter *= 3;      // 第 2 次修改
}

void phase_three() {
    counter -= 4;      // 第 3 次修改
}

void phase_four() {
    counter = counter * counter;  // 第 4 次修改（平方）
}

void helper_a() {
    counter += 1;      // 第 5 次修改
}

void helper_b() {
    counter += 2;      // 第 6 次修改
}

void phase_five() {
    // 这一阶段连续调用两个函数修改 counter
    helper_a();
    helper_b();
}

int main() {
    printf("初始 counter = %d\n", counter);

    phase_one();
    printf("phase_one 之后:   counter = %d\n", counter);

    phase_two();
    printf("phase_two 之后:   counter = %d\n", counter);

    phase_three();
    printf("phase_three 之后: counter = %d\n", counter);

    phase_four();
    printf("phase_four 之后:  counter = %d\n", counter);

    phase_five();
    printf("phase_five 之后:  counter = %d\n", counter);

    printf("\n最终 counter = %d\n", counter);
    return 0;
}

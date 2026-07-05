#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

// 一个最简程序，用于演示 ftrace 的 trace_marker。
// 运行它，然后读取 /sys/kernel/tracing/trace 就能看到你写入的标记。

int main() {
    printf("=== Hello ftrace ===\n");

    // 打开 trace_marker —— 向这个文件写入内容，
    // 就会把自定义消息插入内核的 ftrace 缓冲区。
    int marker = open("/sys/kernel/tracing/trace_marker", O_WRONLY);
    if (marker < 0) {
        perror("open trace_marker");
        return 1;
    }

    // 在工作开始前、进行中、结束后分别写入标记
    write(marker, ">>> DEMO START\n", 15);

    // 进行一些文件操作 —— 这些会触发内核函数，
    // ftrace 也可以捕获到它们
    int fd = open("/tmp/ftrace_hello.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, "Hello from ftrace!\n", 19);
        close(fd);
    }

    // 几个快速的系统调用
    for (int i = 0; i < 5; i++) {
        getpid();
    }

    write(marker, "<<< DEMO END\n", 13);
    close(marker);

    printf("===Done! Now read:  sudo cat /sys/kernel/tracing/trace ===\n");
    return 0;
}
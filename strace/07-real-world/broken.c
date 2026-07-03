#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// ── 实战调试实验 ──
// 这个程序有 3 个不同的"故障"
// 不能用 printf 调试，只能用 strace 找出问题

// 故障 1：读取配置文件失败
void read_config() {
    printf("【场景 1】读取配置文件\n");

    // 尝试打开几个可能的配置文件路径
    const char *paths[] = {
        "/etc/myapp/config.ini",
        "./config.ini",
        "/tmp/config.ini",
        NULL
    };

    for (int i = 0; paths[i] != NULL; i++) {
        int fd = open(paths[i], O_RDONLY);
        if (fd >= 0) {
            char buf[256];
            ssize_t n = read(fd, buf, sizeof(buf) - 1);
            if (n > 0) {
                buf[n] = '\0';
                printf("  找到配置: %s = %s\n", paths[i], buf);
            }
            close(fd);
            return;
        }
        // ★ 关键 bug：没有检查 errno！
        // 如果文件不存在，open 返回 ENOENT
        // 但如果路径中有目录没有执行权限，返回 EACCES
        // 两种情况都被静默忽略了
    }
    printf("  未找到配置文件（可能原因未知）\n");
}

// 故障 2：写入日志文件失败 —— 静默忽略
void write_log(const char *message) {
    // ★ BUG：打开文件用于追加，但如果目录不存在会失败
    int fd = open("/var/log/myapp/app.log", O_WRONLY|O_APPEND|O_CREAT, 0644);

    // ★ 另一个 bug：没有检查 open 的返回值！
    if (fd >= 0) {
        write(fd, message, strlen(message));
        write(fd, "\n", 1);
        close(fd);
    }
    // fd == -1 时静默失败 —— 日志丢了但没人知道
}

// 故障 3：字符串操作触发了不该有的行为
void process_data() {
    printf("【场景 3】处理数据\n");

    // 从"标准输入"读取一个数字
    char input[32];
    printf("  请输入一个数字: ");

    // ★ 在非交互式环境下，/dev/stdin 可能不存在或不可读
    char *result = fgets(input, sizeof(input), stdin);
    if (result == NULL) {
        printf("  读取输入失败\n");
        // strace 会显示 read(0, ...) = -1 以及具体的 errno
        return;
    }

    int value = atoi(input);
    printf("  输入值: %d\n", value);

    // 一个"看似正常"的操作：根据输入打开不同文件
    char filename[64];
    snprintf(filename, sizeof(filename), "/tmp/data_%d.txt", value);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        // ★ 可能的 errno: ENOENT, EACCES, ENOTDIR...
        printf("  无法打开 %s（原因未知）\n", filename);
        return;
    }

    // 假设这里有一些数据处理...
    fclose(fp);
}

int main() {
    printf("╔════════════════════════════════════╗\n");
    printf("║   strace 实战调试 —— 找出 bug     ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    // 场景 1：静默失败的文件查找
    read_config();

    printf("\n");

    // 场景 2：静默失败的日志写入
    write_log("Application started successfully");

    printf("\n");

    // 场景 3：可能因 stdin 问题或路径问题失败
    process_data();

    printf("\n=== 程序结束 ===\n");
    return 0;
}

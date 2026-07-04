#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

// ── 实战调试Experiment ──
// 这个程序有 3 个不同的"故障"
// 不能用 printf 调试，只能用 strace 找出问题

// 故障 1：readconfigfile失败
void read_config() {
    printf("[Scenario 1] Reading configuration file\n");

    // 尝试打开几个可能的configfile路径
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
                printf("  Found config: %s = %s\n", paths[i], buf);
            }
            close(fd);
            return;
        }
        // ★ 关键 bug：没有check errno！
        // 如果file不存在，open return ENOENT
        // 但如果路径中有目录没有执行权限，return EACCES
        // 两种情况都被静默忽略了
    }
    printf("  No config file found (unknown reason)\n");
}

// 故障 2：writelogfile失败 —— 静默忽略
void write_log(const char *message) {
    // ★ BUG：打开file用于追加，但如果目录不存在会失败
    int fd = open("/var/log/myapp/app.log", O_WRONLY|O_APPEND|O_CREAT, 0644);

    // ★ 另一个 bug：没有check open 的return值！
    if (fd >= 0) {
        write(fd, message, strlen(message));
        write(fd, "\n", 1);
        close(fd);
    }
    // fd == -1 时静默失败 —— log丢了但没人知道
}

// 故障 3：stringoperation触发了不该有的行为
void process_data() {
    printf("[Scenario 3] Processing data\n");

    // 从"标准input"read一numbers in total字
    char input[32];
    printf("  Please enter a number: ");

    // ★ 在非交互式环境下，/dev/stdin 可能不存在或不可读
    char *result = fgets(input, sizeof(input), stdin);
    if (result == NULL) {
        printf("  Failed to read input\n");
        // strace 会显示 read(0, ...) = -1 以及具体的 errno
        return;
    }

    int value = atoi(input);
    printf("  Input value: %d\n", value);

    // 一个"看似正常"的operation：根据input打开不同file
    char filename[64];
    snprintf(filename, sizeof(filename), "/tmp/data_%d.txt", value);

    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        // ★ 可能的 errno: ENOENT, EACCES, ENOTDIR...
        printf("  Cannot open %s (unknown reason)\n", filename);
        return;
    }

    // 假设这里有一些dataprocess...
    fclose(fp);
}

int main() {
    printf("╔════════════════════════════════════╗\n");
    printf("║   strace debug practice - find bugs ║\n");
    printf("╚════════════════════════════════════╝\n\n");

    // Scenario 1：静默失败的file查找
    read_config();

    printf("\n");

    // Scenario 2：静默失败的logwrite
    write_log("Application started successfully");

    printf("\n");

    // Scenario 3：可能因 stdin 问题或路径问题失败
    process_data();

    printf("\n=== Program finished ===\n");
    return 0;
}
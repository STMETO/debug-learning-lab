#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// ── 多线程程序：5 个线程 + 1 个共享计数器 ──
// 用来练习查看线程、切换线程、scheduler-locking

int shared_counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// 工作线程 1 & 2：无锁地修改共享变量（有竞态条件！）
void *worker_unsafe(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        // ★ 没有加锁！多个线程同时读写 shared_counter
        int temp = shared_counter;
        usleep(1000);  // 小延迟，增加线程切换概率
        shared_counter = temp + 1;
        printf("[不安全线程 %d] 第 %d 次: shared_counter = %d\n", id, i, shared_counter);
    }
    return NULL;
}

// 工作线程 3 & 4：有锁地修改（正确做法）
void *worker_safe(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mutex);
        int temp = shared_counter;
        usleep(1000);
        shared_counter = temp + 1;
        printf("[安全线程 %d]   第 %d 次: shared_counter = %d\n", id, i, shared_counter);
        pthread_mutex_unlock(&mutex);
        usleep(5000);  // 释放锁后休息一会
    }
    return NULL;
}

// 监控线程：只读 shared_counter，每 10ms 打印一次
void *monitor(void *arg) {
    (void)arg;
    for (int i = 0; i < 10; i++) {
        usleep(10000);  // 10ms
        printf("[监控] shared_counter = %d\n", shared_counter);
    }
    printf("[监控] 监控结束\n");
    return NULL;
}

// 死锁模拟线程（单独使用，需要 --deadlock 参数才启动）
pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;

void *deadlock_a(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lock_a);
    printf("[死锁A] 获取了 lock_a，尝试获取 lock_b...\n");
    usleep(100000);
    pthread_mutex_lock(&lock_b);  // 等 lock_b——但 lock_b 被 B 持有
    printf("[死锁A] 获取了 lock_b\n");
    pthread_mutex_unlock(&lock_b);
    pthread_mutex_unlock(&lock_a);
    return NULL;
}

void *deadlock_b(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lock_b);
    printf("[死锁B] 获取了 lock_b，尝试获取 lock_a...\n");
    usleep(100000);
    pthread_mutex_lock(&lock_a);  // 等 lock_a——但 lock_a 被 A 持有
    printf("[死锁B] 获取了 lock_a\n");
    pthread_mutex_unlock(&lock_a);
    pthread_mutex_unlock(&lock_b);
    return NULL;
}

int main(int argc, char *argv[]) {
    // 如果参数是 --deadlock，运行死锁演示
    if (argc > 1 && strcmp(argv[1], "--deadlock") == 0) {
        printf("=== 死锁演示 ===\n\n");
        pthread_t ta, tb;
        pthread_create(&ta, NULL, deadlock_a, NULL);
        pthread_create(&tb, NULL, deadlock_b, NULL);
        pthread_join(ta, NULL);
        pthread_join(tb, NULL);
        printf("程序结束（如果看到这行，说明没有死锁——这不应该发生）\n");
        return 0;
    }

    printf("=== 多线程调试实验 ===\n");
    printf("共有 5 个线程：2 个不安全 + 2 个安全 + 1 个监控\n\n");

    pthread_t threads[5];
    int ids[4] = {1, 2, 3, 4};

    // 启动线程
    pthread_create(&threads[0], NULL, worker_unsafe, &ids[0]);
    pthread_create(&threads[1], NULL, worker_unsafe, &ids[1]);
    pthread_create(&threads[2], NULL, worker_safe,   &ids[2]);
    pthread_create(&threads[3], NULL, worker_safe,   &ids[3]);
    pthread_create(&threads[4], NULL, monitor,       NULL);

    // 等待所有线程结束
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    printf("\n最终 shared_counter = %d (期望值应该是 20)\n", shared_counter);
    printf("如果 shared_counter != 20，说明不安全线程发生了竞态条件\n");

    return 0;
}

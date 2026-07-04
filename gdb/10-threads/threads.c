#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>

// ── 多线程程序：5 个线程 + 1 个共享计数器 ──
// 用来练习查看线程、切换线程、scheduler-locking

int shared_counter = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Work线程 1 & 2：无锁地修改共享variable（有竞态条件！）
void *worker_unsafe(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        // ★ 没有加锁！多个线程同时读写 shared_counter
        int temp = shared_counter;
        usleep(1000);  // 小延迟，增加线程切换概率
        shared_counter = temp + 1;
        printf("[Unsafe thread %d] iteration %d: shared_counter = %d\n", id, i, shared_counter);
    }
    return NULL;
}

// Work线程 3 & 4：有锁地修改（正确做法）
void *worker_safe(void *arg) {
    int id = *(int *)arg;
    for (int i = 0; i < 5; i++) {
        pthread_mutex_lock(&mutex);
        int temp = shared_counter;
        usleep(1000);
        shared_counter = temp + 1;
        printf("[Safe thread %d]   iteration %d: shared_counter = %d\n", id, i, shared_counter);
        pthread_mutex_unlock(&mutex);
        usleep(5000);  // free锁后休息一会
    }
    return NULL;
}

// Monitor线程：只读 shared_counter，每 10ms 打印一
void *monitor(void *arg) {
    (void)arg;
    for (int i = 0; i < 10; i++) {
        usleep(10000);  // 10ms
        printf("[Monitor] shared_counter = %d\n", shared_counter);
    }
    printf("[Monitor] Monitoring ended\n");
    return NULL;
}

// deadlock模拟线程（单独使用，需要 --deadlock 参数才启动）
pthread_mutex_t lock_a = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lock_b = PTHREAD_MUTEX_INITIALIZER;

void *deadlock_a(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lock_a);
    printf("[Deadlock A] Acquired lock_a, trying to acquire lock_b...\n");
    usleep(100000);
    pthread_mutex_lock(&lock_b);  // 等 lock_b——但 lock_b 被 B 持有
    printf("[Deadlock A] Acquired lock_b\n");
    pthread_mutex_unlock(&lock_b);
    pthread_mutex_unlock(&lock_a);
    return NULL;
}

void *deadlock_b(void *arg) {
    (void)arg;
    pthread_mutex_lock(&lock_b);
    printf("[Deadlock B] Acquired lock_b, trying to acquire lock_a...\n");
    usleep(100000);
    pthread_mutex_lock(&lock_a);  // 等 lock_a——但 lock_a 被 A 持有
    printf("[Deadlock B] Acquired lock_a\n");
    pthread_mutex_unlock(&lock_a);
    pthread_mutex_unlock(&lock_b);
    return NULL;
}

int main(int argc, char *argv[]) {
    // 如果参数Yes --deadlock，Rundeadlock演示
    if (argc > 1 && strcmp(argv[1], "--deadlock") == 0) {
        printf("=== Deadlock Demo ===\n\n");
        pthread_t ta, tb;
        pthread_create(&ta, NULL, deadlock_a, NULL);
        pthread_create(&tb, NULL, deadlock_b, NULL);
        pthread_join(ta, NULL);
        pthread_join(tb, NULL);
        printf("Program ended (if you see this, no deadlock occurred — this should not happen)\n");
        return 0;
    }

    printf("=== Multithreaded Debugging Experiment ===\n");
    printf("5 threads total: 2 unsafe + 2 safe + 1 monitor\n\n");

    pthread_t threads[5];
    int ids[4] = {1, 2, 3, 4};

    // 启动线程
    pthread_create(&threads[0], NULL, worker_unsafe, &ids[0]);
    pthread_create(&threads[1], NULL, worker_unsafe, &ids[1]);
    pthread_create(&threads[2], NULL, worker_safe,   &ids[2]);
    pthread_create(&threads[3], NULL, worker_safe,   &ids[3]);
    pthread_create(&threads[4], NULL, monitor,       NULL);

    // 等待所有线程End
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&mutex);
    printf("\nFinal shared_counter = %d (expected value should be 20)\n", shared_counter);
    printf("If shared_counter != 20, unsafe threads experienced race conditions\n");

    return 0;
}
